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

#ifdef __APPLE__

#import <Cocoa/Cocoa.h>
#include "MacGL.h"
#include <string>

extern "C" void waiveUpdateGLDrawable(uintptr_t wrapperViewHandle)
{
    NSView *wrapper = (__bridge NSView *)(void *)wrapperViewHandle;
    if (wrapper == nil)
        return;

    @autoreleasepool
    {
        // pugl puts its NSOpenGLView as a child of the wrapper view. Its frame lags the window on
        // resize when rendering is driven manually, so force it to fill the (already resized)
        // wrapper bounds and update its context, keeping the GL backing in sync with the window.
        for (NSView *sub in [wrapper subviews])
        {
            if ([sub isKindOfClass:[NSOpenGLView class]])
            {
                [sub setFrame:[wrapper bounds]];
                [[(NSOpenGLView *)sub openGLContext] update];
            }
        }
    }
}

extern "C" void waiveSetCursorHidden(bool hidden)
{
    // NSCursor hide/unhide is a balanced stack; only toggle on real state changes.
    static bool isHidden = false;
    if (hidden == isHidden)
        return;

    isHidden = hidden;

    if (hidden)
        [NSCursor hide];
    else
        [NSCursor unhide];
}

extern "C" const char* waiveGetBundledDataPath()
{
    @autoreleasepool
    {
        NSString *resources = [[NSBundle mainBundle] resourcePath];
        if (resources == nil)
            return "";
        static std::string s;
        s = std::string([resources UTF8String]) + "/WAIVE";
        return s.c_str();
    }
}

extern "C" void waiveToggleFullscreen(uintptr_t wrapperViewHandle)
{
    NSView *view = (__bridge NSView *)(void *)wrapperViewHandle;
    if (view == nil)
        return;

    NSWindow *window = [view window];
    if (window == nil)
        return;

    static bool fullscreen = false;
    static NSRect savedFrame;
    static NSUInteger savedStyle;

    @autoreleasepool
    {
        if (!fullscreen)
        {
            savedFrame = [window frame];
            savedStyle = [window styleMask];

            // Borderless full-screen frame (avoids native macOS fullscreen / a visible title bar).
            [NSApp setPresentationOptions:(NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar)];
            [window setStyleMask:NSWindowStyleMaskBorderless];
            [window setFrame:[[window screen] frame] display:YES];
            [window makeKeyAndOrderFront:nil];
            [window makeFirstResponder:view];
            fullscreen = true;
        }
        else
        {
            [NSApp setPresentationOptions:NSApplicationPresentationDefault];
            [window setStyleMask:savedStyle];
            [window setFrame:savedFrame display:YES];
            fullscreen = false;
        }
    }
}

#endif
