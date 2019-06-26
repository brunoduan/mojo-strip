// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file was generated by:
//   tools/json_schema_compiler/compiler.py.
// NOTE: The format of types has changed. 'FooType' is now
//   'chrome.accessibilityPrivate.FooType'.
// Please run the closure compiler before committing changes.
// See https://chromium.googlesource.com/chromium/src/+/master/docs/closure_compilation.md

/** @fileoverview Externs generated from namespace: accessibilityPrivate */

/**
 * @const
 */
chrome.accessibilityPrivate = {};

/**
 * Information about an alert
 * @typedef {{
 *   message: string
 * }}
 */
chrome.accessibilityPrivate.AlertInfo;

/**
 * Bounding rectangle in global screen coordinates.
 * @typedef {{
 *   left: number,
 *   top: number,
 *   width: number,
 *   height: number
 * }}
 */
chrome.accessibilityPrivate.ScreenRect;

/**
 * @enum {string}
 */
chrome.accessibilityPrivate.Gesture = {
  CLICK: 'click',
  SWIPE_LEFT1: 'swipeLeft1',
  SWIPE_UP1: 'swipeUp1',
  SWIPE_RIGHT1: 'swipeRight1',
  SWIPE_DOWN1: 'swipeDown1',
  SWIPE_LEFT2: 'swipeLeft2',
  SWIPE_UP2: 'swipeUp2',
  SWIPE_RIGHT2: 'swipeRight2',
  SWIPE_DOWN2: 'swipeDown2',
  SWIPE_LEFT3: 'swipeLeft3',
  SWIPE_UP3: 'swipeUp3',
  SWIPE_RIGHT3: 'swipeRight3',
  SWIPE_DOWN3: 'swipeDown3',
  SWIPE_LEFT4: 'swipeLeft4',
  SWIPE_UP4: 'swipeUp4',
  SWIPE_RIGHT4: 'swipeRight4',
  SWIPE_DOWN4: 'swipeDown4',
};

/**
 * @enum {string}
 */
chrome.accessibilityPrivate.SyntheticKeyboardEventType = {
  KEYUP: 'keyup',
  KEYDOWN: 'keydown',
};

/**
 * @enum {string}
 */
chrome.accessibilityPrivate.SyntheticMouseEventType = {
  PRESS: 'press',
  RELEASE: 'release'
};

/**
 * @typedef {{
 *   ctrl: (boolean|undefined),
 *   alt: (boolean|undefined),
 *   search: (boolean|undefined),
 *   shift: (boolean|undefined)
 * }}
 */
chrome.accessibilityPrivate.SyntheticKeyboardModifiers;

/**
 * @typedef {{
 *   type: !chrome.accessibilityPrivate.SyntheticKeyboardEventType,
 *   keyCode: number,
 *   modifiers: (!chrome.accessibilityPrivate.SyntheticKeyboardModifiers|undefined)
 * }}
 */
chrome.accessibilityPrivate.SyntheticKeyboardEvent;

/**
 * @typedef {{
 *   type: !chrome.accessibilityPrivate.SyntheticMouseEventType,
 *   x: number,
 *   y: number
 * }}
 */
chrome.accessibilityPrivate.SyntheticMouseEvent;

/**
 * @enum {string}
 */
chrome.accessibilityPrivate.KeyboardMode = {
  FULL_WIDTH: 'FULL_WIDTH',
  FLOATING: 'FLOATING',
};

/**
 * @enum {string}
 */
chrome.accessibilityPrivate.SelectToSpeakState = {
  SELECTING: 'selecting',
  SPEAKING: 'speaking',
  INACTIVE: 'inactive',
};

/**
 * Enables or disables native accessibility support. Once disabled, it is up to
 * the calling extension to provide accessibility for web contents.
 * @param {boolean} enabled True if native accessibility support should be
 *     enabled.
 */
chrome.accessibilityPrivate.setNativeAccessibilityEnabled = function(enabled) {};

/**
 * Sets the bounds of the accessibility focus ring.
 * @param {!Array<!chrome.accessibilityPrivate.ScreenRect>} rects Array of
 *     rectangles to draw the accessibility focus ring around.
 * @param {string=} color CSS-style hex color string beginning with # like
 *     #FF9982 or #EEE.
 */
chrome.accessibilityPrivate.setFocusRing = function(rects, color) {};

/**
 * Sets the bounds of the accessibility highlight.
 * @param {!Array<!chrome.accessibilityPrivate.ScreenRect>} rects Array of
 *     rectangles to draw the highlight around.
 * @param {string} color CSS-style hex color string beginning with # like
 *     #FF9982 or #EEE.
 */
chrome.accessibilityPrivate.setHighlights = function(rects, color) {};

/**
 * Sets the calling extension as a listener of all keyboard events optionally
 * allowing the calling extension to capture/swallow the key event via DOM apis.
 * Returns false via callback when unable to set the listener.
 * @param {boolean} enabled True if the caller wants to listen to key events;
 *     false to stop listening to events. Note that there is only ever one
 *     extension listening to key events.
 * @param {boolean} capture True if key events should be swallowed natively and
 *     not propagated if preventDefault() gets called by the extension's
 *     background page.
 */
chrome.accessibilityPrivate.setKeyboardListener = function(enabled, capture) {};

/**
 * Darkens or undarkens the screen.
 * @param {boolean} enabled True to darken screen; false to undarken screen.
 */
chrome.accessibilityPrivate.darkenScreen = function(enabled) {};

/**
 * Change the keyboard keys captured by Switch Access.
 * @param {!Array<number>} key_codes The key codes for the keys that will be
 *     captured.
 */
chrome.accessibilityPrivate.setSwitchAccessKeys = function(key_codes) {};

/**
 * Sets current ARC app to use native ARC support.
 * @param {boolean} enabled True for ChromeVox (native), false for TalkBack.
 */
chrome.accessibilityPrivate.setNativeChromeVoxArcSupportForCurrentApp = function(enabled) {};

/**
 * Sends a fabricated key event.
 * @param {!chrome.accessibilityPrivate.SyntheticKeyboardEvent} keyEvent The
 *     event to send.
 */
chrome.accessibilityPrivate.sendSyntheticKeyEvent = function(keyEvent) {};

/**
 * Enables or disables mouse events in ChromeVox.
 * @param {boolean} enabled True if ChromeVox should receive mouse events.
 */
chrome.accessibilityPrivate.enableChromeVoxMouseEvents = function(enabled) {};

/**
 * Sends a fabricated mouse event.
 * @param {!chrome.accessibilityPrivate.SyntheticMouseEvent} mouseEvent The
 *     event to send.
 */
chrome.accessibilityPrivate.sendSyntheticMouseEvent = function(mouseEvent) {};

/**
 * Fired whenever ChromeVox should output introduction.
 * @type {!ChromeEvent}
 */
chrome.accessibilityPrivate.onIntroduceChromeVox;

/**
 * Fired when an accessibility gesture is detected by the touch exploration
 * controller.
 * @type {!ChromeEvent}
 */
chrome.accessibilityPrivate.onAccessibilityGesture;

/**
 * Fired when we first detect two fingers are held down, which can be used to
 * toggle spoken feedback on some touch-only devices.
 * @type {!ChromeEvent}
 */
chrome.accessibilityPrivate.onTwoFingerTouchStart;

/**
 * Fired when  the user is no longer holding down two fingers (including
 * releasing one, holding down three, or moving them).
 * @type {!ChromeEvent}
 */
chrome.accessibilityPrivate.onTwoFingerTouchStop;

/**
 * Called by the Select-to-Speak extension when Select-to-Speak has changed states,
 * between selecting with the mouse, speaking, and inactive.
 * @param {!chrome.accessibilityPrivate.SelectToSpeakState} state The current
 *    state of the Select-to-Speak extension
 */
chrome.accessibilityPrivate.onSelectToSpeakStateChanged = function(state) {};

/**
 * Called when Chrome OS wants to toggle the Select-to-Speak state, between selecting
 * with the mouse, speaking, and inactive
 * @type {!ChromeEvent}
 */
chrome.accessibilityPrivate.onSelectToSpeakStateChangeRequested;

/**
 * Called when a Switch Access user activates dictation from the context menu.
 */
chrome.accessibilityPrivate.toggleDictation = function() {};
