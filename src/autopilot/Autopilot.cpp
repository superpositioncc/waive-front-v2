/*
WAIVE-FRONT
Copyright (C) 2024  Bram Bogaerts, Superposition

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef AUTOPILOT_CPP
#define AUTOPILOT_CPP

#include "DistrhoPluginInfo.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

/**
 * @brief The autopilot keeps the visualisation changing by itself
 *
 * Ported from Bram Bogaerts' automatic mode on the standalone branch. Each visual parameter
 * follows a driver: a slow sine, a pulse that falls off, or smoothed noise. At Speed 1, every 8
 * to 24 seconds a parameter switches to another driver. Items, categories and layers change on
 * their own random clocks; categories come from a shuffled deck, so every category comes up
 * before any comes up again.
 *
 * A manual change, host automation or an OSC message holds what it changed for HoldSeconds.
 * After that the autopilot takes over again, gliding back over GlideSeconds.
 */
class Autopilot
{
public:
	/** The visual parameters the autopilot drives; the background is one group of three */
	enum Group
	{
		AP_Blur,
		AP_Focus,
		AP_Space,
		AP_Zoom,
		AP_Background,
		AP_Count
	};

	static constexpr int NumLayers = 3;
	static constexpr double HoldSeconds = 8.0;	/**< How long a manual change or OSC message holds */
	static constexpr double GlideSeconds = 2.0; /**< How long the autopilot takes to glide back */

	float speed = 1.0f;	 /**< How fast everything changes; 1 is the pace of the original automatic mode */
	float chaos = 0.35f; /**< How far the visual parameters swing: 0 is calm, 1 is the full range */

	/** What the autopilot changes about the content in one frame */
	struct Moves
	{
		bool newItem[NumLayers] = {false, false, false};	 /**< Play another item from the same category */
		bool newCategory[NumLayers] = {false, false, false}; /**< Play an item from the next category */
		int toggleLayer = -1;								 /**< The layer to switch on or off, or -1 */
		bool toggleOn = false;								 /**< Whether to switch it on */
	};

	bool isOn() const
	{
		return on;
	}

	/**
	 * @brief Start from the picture as it is: the parameters glide from their current values
	 *
	 * @param now The time in seconds
	 */
	void start(double now)
	{
		on = true;
		lastReal = now;
		structClock = 0.0;

		// A spread of drivers and periods, so there is variety right away
		const int startDriver[AP_Count] = {DRIVER_PULSE, DRIVER_LFO, DRIVER_NOISE, DRIVER_LFO, DRIVER_LFO};
		const double startPeriod[AP_Count] = {12.0, 14.0, 11.0, 18.0, 23.0};

		for (int g = 0; g < AP_Count; g++)
		{
			DriverState &s = drivers[g];
			s.clock = 0.0;
			s.driver = startDriver[g];
			s.lfoPeriod = startPeriod[g];
			s.lastBeat = 0.0;
			s.nextBeat = 0.0;
			s.noiseFrom = randf();
			s.noiseTo = randf();
			s.noiseStart = 0.0;
			s.noiseDur = randf(0.5f, 2.0f);
			s.nextSwitch = randf(6.0f, 18.0f);

			groups[g].holdUntil = now;
			groups[g].glidePending = true;
		}

		for (int i = 0; i < NumLayers; i++)
		{
			layerHoldUntil[i] = now;
			nextItemChange[i] = randf(0.0f, 4.0f);
			nextCategoryChange[i] = randf(2.0f, 8.0f);
		}

		nextLayerToggle = randf(4.0f, 10.0f);
	}

	void stop()
	{
		on = false;
	}

	/**
	 * @brief Hold a group of visual parameters: the autopilot leaves it alone for a while
	 */
	void holdGroup(int group, double now)
	{
		groups[group].holdUntil = now + HoldSeconds;
		groups[group].glidePending = true;
	}

	/**
	 * @brief Hold a layer: the autopilot does not change its content or switch it for a while
	 */
	void holdLayer(int layer, double now)
	{
		layerHoldUntil[layer] = now + HoldSeconds;
	}

	bool layerHeld(int layer, double now) const
	{
		return now < layerHoldUntil[layer];
	}

	/**
	 * @brief The group of visual parameters a plugin parameter belongs to
	 *
	 * @return int The group, or -1
	 */
	static int groupOf(uint32_t index)
	{
		switch (index)
		{
		case BlurSize:
			return AP_Blur;
		case FocusDistance:
			return AP_Focus;
		case Parameters::Space:
			return AP_Space;
		case Parameters::Zoom:
			return AP_Zoom;
		case BackgroundHue:
		case BackgroundSaturation:
		case BackgroundValue:
			return AP_Background;
		default:
			return -1;
		}
	}

	/**
	 * @brief The layer a plugin parameter changes the content of
	 *
	 * @return int The layer, or -1
	 */
	static int layerOf(uint32_t index)
	{
		if (index >= EnableLayer1 && index <= EnableLayer3)
			return index - EnableLayer1;
		if (index >= RandomizeCategory1 && index <= RandomizeCategory3)
			return index - RandomizeCategory1;
		if (index >= RandomizeItem1 && index <= RandomizeItem3)
			return index - RandomizeItem1;
		return -1;
	}

	/**
	 * @brief Drive the visual parameters and decide what to change about the content
	 *
	 * Writes the visual parameters straight into the array the viewer draws from; the host is
	 * not told, so the autopilot leaves no automation behind.
	 *
	 * @param now The time in seconds
	 * @param parameters The plugin parameters
	 * @param layersEnabled Which layers are on
	 * @return Moves The changes to the content for the caller to make
	 */
	Moves update(double now, float *parameters, const std::vector<bool> &layersEnabled)
	{
		// Each parameter runs on its own clock and switches driver now and then. Content changes
		// run on a steady clock. At speed 1 both run at the pace of the original automatic mode.
		// Long pauses, such as a computer waking from sleep, count as one short frame.
		const double dt = std::min(std::max(now - lastReal, 0.0), 0.25);
		lastReal = now;

		for (int g = 0; g < AP_Count; g++)
		{
			drivers[g].clock += dt * speed * 0.75;
			maybeSwitchDriver(drivers[g]);
		}
		structClock += dt * speed;

		// Chaos pulls focus and space toward the centre, and blur, zoom and background toward zero
		float target[AP_Count][3] = {};
		target[AP_Blur][0] = mapTowardZero(driverValue(drivers[AP_Blur]), 0.0f, 0.65f);
		target[AP_Focus][0] = mapTowardCentre(driverValue(drivers[AP_Focus]), 0.1f, 0.9f);
		target[AP_Space][0] = mapTowardCentre(driverValue(drivers[AP_Space]), 0.02f, 0.18f);
		target[AP_Zoom][0] = mapTowardZero(driverValue(drivers[AP_Zoom]), 0.0f, 0.7f);

		const float background = driverValue(drivers[AP_Background]);
		target[AP_Background][0] = (float)std::fmod(drivers[AP_Background].clock * 0.03, 1.0);
		target[AP_Background][1] = mapTowardZero(background, 0.0f, 1.0f);
		target[AP_Background][2] = mapTowardZero(background, 0.0f, 1.0f);

		for (int g = 0; g < AP_Count; g++)
			applyGroup(g, target[g], now, parameters);

		Moves moves;
		const double ts = structClock;

		for (int i = 0; i < NumLayers; i++)
		{
			// While held, keep the next change at least a little way off, so it comes a few
			// seconds after the hold ends
			if (layerHeld(i, now))
			{
				if (nextItemChange[i] < ts + 2.0)
					nextItemChange[i] = ts + randf(2.0f, 7.0f);
				if (nextCategoryChange[i] < ts + 6.0)
					nextCategoryChange[i] = ts + randf(6.0f, 16.0f);
				continue;
			}

			if (ts >= nextItemChange[i])
			{
				moves.newItem[i] = layersEnabled[i];
				nextItemChange[i] = ts + randf(2.0f, 7.0f);
			}

			if (ts >= nextCategoryChange[i])
			{
				moves.newCategory[i] = layersEnabled[i];
				nextCategoryChange[i] = ts + randf(6.0f, 16.0f);
			}
		}

		if (ts >= nextLayerToggle)
		{
			nextLayerToggle = ts + randf(4.0f, 10.0f);
			chooseLayerToggle(now, layersEnabled, moves);
		}

		return moves;
	}

	/**
	 * @brief The next category, from a shuffled deck: all categories come up once before any repeats
	 *
	 * @param count The number of categories
	 * @return int The index of the category, or -1 when there are none
	 */
	int drawCategory(int count)
	{
		if (count <= 0)
			return -1;

		if (deckSize != count)
		{
			deck.clear();
			deckSize = count;
		}

		if (deck.empty())
		{
			for (int i = 0; i < count; i++)
				deck.push_back(i);

			for (int i = count - 1; i > 0; i--)
				std::swap(deck[i], deck[std::rand() % (i + 1)]);

			// Not the same category twice in a row across a reshuffle
			if (count > 1 && deck.back() == lastDrawn)
				std::swap(deck.front(), deck.back());
		}

		lastDrawn = deck.back();
		deck.pop_back();
		return lastDrawn;
	}

	/**
	 * @brief Read the settings the standalone app saved
	 *
	 * @return true if the autopilot was on when they were saved
	 */
	bool load()
	{
		const std::filesystem::path path = settingsPath();
		if (path.empty())
			return false;

		std::ifstream file(path);
		std::string key;
		float value;
		bool wasOn = false;

		while (file >> key >> value)
		{
			if (key == "on")
				wasOn = value != 0.0f;
			else if (key == "speed" && std::isfinite(value))
				speed = std::min(std::max(value, 0.1f), 4.0f);
			else if (key == "chaos" && std::isfinite(value))
				chaos = std::min(std::max(value, 0.0f), 1.0f);
		}

		return wasOn;
	}

	/**
	 * @brief Save the settings of the standalone app, so it starts the same way next time
	 */
	void save() const
	{
		const std::filesystem::path path = settingsPath();
		if (path.empty())
			return;

		std::error_code error;
		std::filesystem::create_directories(path.parent_path(), error);

		std::ofstream file(path);
		file << "on " << (on ? 1 : 0) << "\n";
		file << "speed " << speed << "\n";
		file << "chaos " << chaos << "\n";
	}

	/**
	 * @brief Where the standalone app keeps its autopilot settings
	 *
	 * macOS: ~/Library/Application Support/WAIVE-FRONT V2/autopilot.txt
	 * Windows: %APPDATA%\WAIVE-FRONT V2\autopilot.txt
	 */
	static std::filesystem::path settingsPath()
	{
#if defined(_WIN32)
		const char *base = std::getenv("APPDATA");
		if (base == nullptr)
			return {};
		return std::filesystem::path(base) / DISTRHO_PLUGIN_NAME / "autopilot.txt";
#else
		const char *home = std::getenv("HOME");
		if (home == nullptr)
			return {};
#if defined(__APPLE__)
		return std::filesystem::path(home) / "Library" / "Application Support" / DISTRHO_PLUGIN_NAME / "autopilot.txt";
#else
		return std::filesystem::path(home) / ".config" / DISTRHO_PLUGIN_NAME / "autopilot.txt";
#endif
#endif
	}

private:
	/** The drivers a parameter can follow; each parameter switches between them over time */
	enum Driver
	{
		DRIVER_LFO,	  /**< Slow sine */
		DRIVER_PULSE, /**< Pulse that falls off */
		DRIVER_NOISE, /**< Smoothed random drift */
		DRIVER_COUNT
	};

	/** A parameter's driver and the phase of each driver */
	struct DriverState
	{
		double clock = 0.0;		 /**< The time this parameter runs on */
		int driver = DRIVER_LFO; /**< The driver it follows now */
		double nextSwitch = 0.0; /**< When to switch to another driver */
		double lfoPeriod = 12.0; /**< Sine: period in seconds */
		double lastBeat = 0.0;	 /**< Pulse: time of the last beat */
		double nextBeat = 0.0;	 /**< Pulse: time of the next beat */
		float noiseFrom = 0.5f;	 /**< Noise: start of the current segment */
		float noiseTo = 0.5f;	 /**< Noise: end of the current segment */
		double noiseStart = 0.0; /**< Noise: when the current segment started */
		double noiseDur = 1.0;	 /**< Noise: length of the current segment */
	};

	/** Holding and gliding back, per group of visual parameters */
	struct GroupState
	{
		double holdUntil = 0.0;	   /**< Leave the group alone until then */
		bool glidePending = false; /**< Glide back when the hold ends */
		double glideStart = -1.0;  /**< When the glide started */
		float from[3] = {0, 0, 0}; /**< The values the glide starts from */
	};

	bool on = false;
	double lastReal = 0.0;	  /**< The time at the previous frame */
	double structClock = 0.0; /**< The clock for content changes */
	DriverState drivers[AP_Count];
	GroupState groups[AP_Count];
	double layerHoldUntil[NumLayers] = {0, 0, 0};
	double nextItemChange[NumLayers] = {0, 0, 0};
	double nextCategoryChange[NumLayers] = {0, 0, 0};
	double nextLayerToggle = 0.0;

	std::vector<int> deck; /**< Categories still to come in this round */
	int deckSize = 0;	   /**< The number of categories the deck was made for */
	int lastDrawn = -1;	   /**< The category drawn last */

	/** The plugin parameters in each group */
	static int groupSize(int group)
	{
		return group == AP_Background ? 3 : 1;
	}

	static uint32_t groupParameter(int group, int k)
	{
		switch (group)
		{
		case AP_Blur:
			return BlurSize;
		case AP_Focus:
			return FocusDistance;
		case AP_Space:
			return Parameters::Space;
		case AP_Zoom:
			return Parameters::Zoom;
		default:
			return BackgroundHue + k;
		}
	}

	/**
	 * @brief Write a group's driven values, unless it is held; glide back after a hold
	 */
	void applyGroup(int g, const float *target, double now, float *parameters)
	{
		GroupState &s = groups[g];

		if (now < s.holdUntil)
			return;

		if (s.glidePending)
		{
			s.glidePending = false;
			s.glideStart = now;

			for (int k = 0; k < groupSize(g); k++)
			{
				const float value = parameters[groupParameter(g, k)];
				s.from[k] = std::isfinite(value) ? std::min(std::max(value, 0.0f), 1.0f) : target[k];
			}
		}

		float w = (float)((now - s.glideStart) / GlideSeconds);
		w = std::min(std::max(w, 0.0f), 1.0f);
		w = w * w * (3.0f - 2.0f * w);

		for (int k = 0; k < groupSize(g); k++)
		{
			float delta = target[k] - s.from[k];

			// The hue goes round: take the short way
			if (g == AP_Background && k == 0)
			{
				if (delta > 0.5f)
					delta -= 1.0f;
				else if (delta < -0.5f)
					delta += 1.0f;

				float hue = s.from[k] + delta * w;
				parameters[groupParameter(g, k)] = hue - std::floor(hue);
			}
			else
			{
				parameters[groupParameter(g, k)] = s.from[k] + delta * w;
			}
		}
	}

	/**
	 * @brief Switch a random layer that is not held, keeping at least one layer on
	 */
	void chooseLayerToggle(double now, const std::vector<bool> &layersEnabled, Moves &moves)
	{
		std::vector<int> free;
		int enabled = 0;

		for (int i = 0; i < NumLayers; i++)
		{
			if (!layerHeld(i, now))
				free.push_back(i);
			if (layersEnabled[i])
				enabled++;
		}

		if (free.empty())
			return;

		int layer = free[std::rand() % free.size()];
		bool switchOn = !layersEnabled[layer];

		// The last layer that is on stays on; another layer comes on
		if (!switchOn && enabled <= 1)
		{
			std::vector<int> off;
			for (int i : free)
				if (!layersEnabled[i])
					off.push_back(i);

			if (off.empty())
				return;

			layer = off[std::rand() % off.size()];
			switchOn = true;
		}

		moves.toggleLayer = layer;
		moves.toggleOn = switchOn;
	}

	/** A random float in [0, 1] */
	static float randf()
	{
		return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX);
	}

	/** A random float in [lo, hi] */
	static float randf(float lo, float hi)
	{
		return lo + (hi - lo) * randf();
	}

	static float clampf(float v, float lo, float hi)
	{
		return std::max(lo, std::min(v, hi));
	}

	/** A sine at the given period in seconds, in [0, 1] */
	static float lfo01(double t, double period)
	{
		const double pi = 3.14159265358979323846; // M_PI is not standard; MSVC hides it
		return 0.5f + 0.5f * (float)std::sin(2.0 * pi * (t / period));
	}

	/** Map [0, 1] into [lo, hi], toward lo as chaos goes to 0 */
	float mapTowardZero(float v01, float lo, float hi) const
	{
		return lo + (hi - lo) * v01 * chaos;
	}

	/** Map [0, 1] into [lo, hi], toward the centre as chaos goes to 0 */
	float mapTowardCentre(float v01, float lo, float hi) const
	{
		return lo + (hi - lo) * (0.5f + (v01 - 0.5f) * chaos);
	}

	/** The value of a parameter's driver in [0, 1]; advances the driver's phase */
	float driverValue(DriverState &s)
	{
		const double t = s.clock;

		switch (s.driver)
		{
		case DRIVER_PULSE:
		{
			if (t >= s.nextBeat)
			{
				s.lastBeat = t;
				s.nextBeat = t + randf(0.35f, 0.9f);
			}
			return clampf((float)std::exp(-6.0 * (t - s.lastBeat)), 0.0f, 1.0f);
		}
		case DRIVER_NOISE:
		{
			if (t >= s.noiseStart + s.noiseDur)
			{
				s.noiseFrom = s.noiseTo;
				s.noiseTo = randf();
				s.noiseStart = t;
				s.noiseDur = randf(0.5f, 2.0f);
			}
			float x = clampf((float)((t - s.noiseStart) / s.noiseDur), 0.0f, 1.0f);
			return s.noiseFrom + (s.noiseTo - s.noiseFrom) * (x * x * (3.0f - 2.0f * x));
		}
		case DRIVER_LFO:
		default:
			return lfo01(t, s.lfoPeriod);
		}
	}

	/** Now and then, switch a parameter to another driver, starting it cleanly */
	void maybeSwitchDriver(DriverState &s)
	{
		if (s.clock < s.nextSwitch)
			return;

		int next = std::rand() % DRIVER_COUNT;
		if (next == s.driver)
			next = (next + 1) % DRIVER_COUNT;
		s.driver = next;

		s.lfoPeriod = randf(8.0f, 26.0f);
		s.lastBeat = s.clock;
		s.nextBeat = s.clock;
		s.noiseFrom = s.noiseTo;
		s.noiseTo = randf();
		s.noiseStart = s.clock;
		s.noiseDur = randf(0.5f, 2.0f);
		s.nextSwitch = s.clock + randf(6.0f, 18.0f);
	}
};

#endif
