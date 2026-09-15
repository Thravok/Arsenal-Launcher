// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 Kenneth Chew <79120643+kthchew@users.noreply.github.com>
 *  Copyright (C) 2026 Arsenal Launcher Contributors
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the Free
 *  Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ThemeManager.h"

#include <AppKit/AppKit.h>
#include <QApplication>
#include <QMainWindow>
#include <QWidget>

static NSString* const kArsenalVibrancyIdentifier = @"arsenal.window.vibrancy";

static NSVisualEffectView* findVibrancyViewIn(NSView* parent)
{
    if (!parent) {
        return nil;
    }
    for (NSView* subview in [parent subviews]) {
        if ([subview isKindOfClass:[NSVisualEffectView class]] &&
            [subview.identifier isEqualToString:kArsenalVibrancyIdentifier]) {
            return (NSVisualEffectView*)subview;
        }
    }
    return nil;
}

static void removeGlassBackdropsIn(NSView* parent)
{
    if (!parent) {
        return;
    }
    // Also strip any NSGlassEffectView left behind by earlier experiments.
    NSArray<NSView*>* subviews = [parent.subviews copy];
    for (NSView* subview in subviews) {
        if ([subview.identifier isEqualToString:kArsenalVibrancyIdentifier]) {
            [subview removeFromSuperview];
        }
    }
}

static void restoreStandardTitlebar(NSWindow* window, QColor color)
{
    if (!window) {
        return;
    }
    NSWindowStyleMask mask = window.styleMask;
    mask &= ~NSWindowStyleMaskFullSizeContentView;
    window.styleMask = mask;
    window.titleVisibility = NSWindowTitleVisible;
    window.titlebarAppearsTransparent = YES;
    window.movableByWindowBackground = NO;
    window.opaque = YES;
    window.backgroundColor = [NSColor colorWithRed:color.redF() green:color.greenF() blue:color.blueF() alpha:1.0];
}

void ThemeManager::setTitlebarColorOnMac(WId windowId, QColor color)
{
    if (windowId == 0) {
        return;
    }

    NSView* view = (NSView*)windowId;
    NSWindow* window = [view window];
    if (!window) {
        return;
    }

    // Keep a normal, draggable titlebar. Only tint it to match the theme.
    NSWindowStyleMask mask = window.styleMask;
    mask &= ~NSWindowStyleMaskFullSizeContentView;
    window.styleMask = mask;
    window.titleVisibility = NSWindowTitleVisible;
    window.titlebarAppearsTransparent = YES;
    window.movableByWindowBackground = NO;
    window.opaque = YES;
    window.backgroundColor = [NSColor colorWithRed:color.redF() green:color.greenF() blue:color.blueF() alpha:1.0];
}

void ThemeManager::setTitlebarColorOfAllWindowsOnMac(QColor color)
{
    NSArray<NSWindow*>* windows = [NSApp windows];
    for (NSWindow* window : windows) {
        setTitlebarColorOnMac((WId)window.contentView, color);
    }

    // We want to change the titlebar color of newly opened windows as well.
    // There's no notification for when a new window is opened, but we can set the color when a window switches
    // from occluded to visible, which also fires on open.
    NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
    stopSettingNewWindowColorsOnMac();
    m_windowTitlebarObserver = [center addObserverForName:NSWindowDidChangeOcclusionStateNotification
                                                   object:nil
                                                    queue:[NSOperationQueue mainQueue]
                                               usingBlock:^(NSNotification* notification) {
                                                   NSWindow* window = notification.object;
                                                   setTitlebarColorOnMac((WId)window.contentView, color);
                                                   if (m_windowVibrancyEnabled) {
                                                       setWindowVibrancyOnMac((WId)window.contentView, true, m_windowVibrancyDark);
                                                   }
                                               }];
}

void ThemeManager::stopSettingNewWindowColorsOnMac()
{
    if (m_windowTitlebarObserver) {
        NSNotificationCenter* center = [NSNotificationCenter defaultCenter];
        [center removeObserver:m_windowTitlebarObserver];
        m_windowTitlebarObserver = nil;
    }
}

void ThemeManager::setWindowVibrancyOnMac(WId windowId, bool enabled, bool darkAppearance)
{
    if (windowId == 0) {
        return;
    }

    NSView* view = (NSView*)windowId;
    NSWindow* window = [view window];
    if (!window) {
        return;
    }

    NSView* contentView = [window contentView];
    if (!contentView) {
        return;
    }

    // Qt draws into the content view's layer. A child effect view would composite
    // on top of that layer and hide widgets — place vibrancy as a sibling behind it.
    NSView* container = [contentView superview];
    removeGlassBackdropsIn(contentView);
    NSVisualEffectView* effectView = findVibrancyViewIn(container);
    // Clean up non-visual-effect backdrops from earlier builds.
    if (container) {
        NSArray<NSView*>* siblings = [container.subviews copy];
        for (NSView* subview in siblings) {
            if ([subview.identifier isEqualToString:kArsenalVibrancyIdentifier] &&
                ![subview isKindOfClass:[NSVisualEffectView class]]) {
                [subview removeFromSuperview];
            }
        }
        effectView = findVibrancyViewIn(container);
    }

    const QColor windowColor = qApp->palette().color(QPalette::Window);

    if (!enabled) {
        [effectView removeFromSuperview];
        restoreStandardTitlebar(window, windowColor);
        return;
    }

    // Stable chrome: keep the system titlebar for dragging; only tint + subtle blur.
    restoreStandardTitlebar(window, windowColor);

    if (!container) {
        return;
    }

    if (!effectView) {
        effectView = [[NSVisualEffectView alloc] initWithFrame:contentView.frame];
        effectView.identifier = kArsenalVibrancyIdentifier;
        effectView.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        effectView.blendingMode = NSVisualEffectBlendingModeBehindWindow;
        effectView.state = NSVisualEffectStateActive;
        [container addSubview:effectView positioned:NSWindowBelow relativeTo:contentView];
    } else {
        effectView.frame = contentView.frame;
        [container addSubview:effectView positioned:NSWindowBelow relativeTo:contentView];
    }

    effectView.material = NSVisualEffectMaterialUnderWindowBackground;
    if (@available(macOS 10.14, *)) {
        effectView.appearance =
            [NSAppearance appearanceNamed:(darkAppearance ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua)];
    }
}

void ThemeManager::enableWindowVibrancyOnMac(bool enabled)
{
    m_windowVibrancyEnabled = enabled;
    const QColor windowColor = qApp->palette().color(QPalette::Window);
    m_windowVibrancyDark = windowColor.lightnessF() < 0.5;

    const auto topLevels = QApplication::topLevelWidgets();
    for (QWidget* widget : topLevels) {
        if (!widget->isWindow()) {
            continue;
        }
        // Only main windows get the backdrop. Dialogs stay solid so overlays don't
        // composite through each other (which looked like a broken layout).
        const bool isMainChrome = qobject_cast<QMainWindow*>(widget) != nullptr;
        const bool useGlass = enabled && isMainChrome;

        widget->setAttribute(Qt::WA_TranslucentBackground, useGlass);
        widget->setAttribute(Qt::WA_NoSystemBackground, useGlass);
        const WId wid = widget->winId();
        setWindowVibrancyOnMac(wid, useGlass, m_windowVibrancyDark);
        if (!useGlass) {
            setTitlebarColorOnMac(wid, windowColor);
        } else {
            widget->update();
        }
    }
}
