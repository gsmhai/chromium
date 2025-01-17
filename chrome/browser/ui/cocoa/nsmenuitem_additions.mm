// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/nsmenuitem_additions.h"

#include <Carbon/Carbon.h>

#include "base/logging.h"
#include "base/mac/scoped_cftyperef.h"
#include "ui/events/keycodes/keyboard_code_conversion_mac.h"

namespace {
bool g_is_input_source_dvorak_qwerty = false;
bool g_is_input_source_czech = false;
}  // namespace

void SetIsInputSourceDvorakQwertyForTesting(bool is_dvorak_qwerty) {
  g_is_input_source_dvorak_qwerty = is_dvorak_qwerty;
}

void SetIsInputSourceCzechForTesting(bool is_czech) {
  g_is_input_source_czech = is_czech;
}

@interface KeyboardInputSourceListener : NSObject
@end

@implementation KeyboardInputSourceListener

- (instancetype)init {
  if (self = [super init]) {
    [[NSNotificationCenter defaultCenter]
        addObserver:self
           selector:@selector(inputSourceDidChange:)
               name:NSTextInputContextKeyboardSelectionDidChangeNotification
             object:nil];
    [self updateInputSource];
  }
  return self;
}

- (void)dealloc {
  [[NSNotificationCenter defaultCenter] removeObserver:self];
  [super dealloc];
}

- (void)updateInputSource {
  base::ScopedCFTypeRef<TISInputSourceRef> inputSource(
      TISCopyCurrentKeyboardInputSource());
  NSString* inputSourceID = (NSString*)TISGetInputSourceProperty(
      inputSource.get(), kTISPropertyInputSourceID);
  g_is_input_source_dvorak_qwerty =
      [inputSourceID isEqualToString:@"com.apple.keylayout.DVORAK-QWERTYCMD"];
  g_is_input_source_czech =
      [inputSourceID rangeOfString:@"com.apple.keylayout.Czech"].location !=
      NSNotFound;
}

- (void)inputSourceDidChange:(NSNotification*)notification {
  [self updateInputSource];
}

@end

@implementation NSMenuItem(ChromeAdditions)

- (BOOL)cr_firesForKeyEvent:(NSEvent*)event {
  if (![self isEnabled])
    return NO;

  DCHECK([event type] == NSKeyDown);
  // In System Preferences->Keyboard->Keyboard Shortcuts, it is possible to add
  // arbitrary keyboard shortcuts to applications. It is not documented how this
  // works in detail, but |NSMenuItem| has a method |userKeyEquivalent| that
  // sounds related.
  // However, it looks like |userKeyEquivalent| is equal to |keyEquivalent| when
  // a user shortcut is set in system preferences, i.e. Cocoa automatically
  // sets/overwrites |keyEquivalent| as well. Hence, this method can ignore
  // |userKeyEquivalent| and check |keyEquivalent| only.

  // Menu item key equivalents are nearly all stored without modifiers. The
  // exception is shift, which is included in the key and not in the modifiers
  // for printable characters (but not for stuff like arrow keys etc).
  NSString* eventString = [event charactersIgnoringModifiers];
  NSUInteger eventModifiers =
      [event modifierFlags] & NSDeviceIndependentModifierFlagsMask;

  // cmd-opt-a gives some weird char as characters and "a" as
  // charactersWithoutModifiers with an US layout, but an "a" as characters and
  // a weird char as "charactersWithoutModifiers" with a cyrillic layout. Oh,
  // Cocoa! Instead of getting the current layout from Text Input Services,
  // and then requesting the kTISPropertyUnicodeKeyLayoutData and looking in
  // there, let's try a pragmatic hack.
  if ([eventString length] == 0 ||
      ([eventString characterAtIndex:0] > 0x7f &&
       [[event characters] length] > 0 &&
       [[event characters] characterAtIndex:0] <= 0x7f)) {
    eventString = [event characters];

    // Process the shift if necessary.
    if (eventModifiers & NSShiftKeyMask)
      eventString = [eventString uppercaseString];
  }

  if ([eventString length] == 0 || [[self keyEquivalent] length] == 0)
    return NO;

  // Turns out esc never fires unless cmd or ctrl is down.
  if ([event keyCode] == kVK_Escape &&
      (eventModifiers & (NSControlKeyMask | NSCommandKeyMask)) == 0)
    return NO;

  // From the |NSMenuItem setKeyEquivalent:| documentation:
  //
  // If you want to specify the Backspace key as the key equivalent for a menu
  // item, use a single character string with NSBackspaceCharacter (defined in
  // NSText.h as 0x08) and for the Forward Delete key, use NSDeleteCharacter
  // (defined in NSText.h as 0x7F). Note that these are not the same characters
  // you get from an NSEvent key-down event when pressing those keys.
  if ([[self keyEquivalent] characterAtIndex:0] == NSBackspaceCharacter
      && [eventString characterAtIndex:0] == NSDeleteCharacter) {
    unichar chr = NSBackspaceCharacter;
    eventString = [NSString stringWithCharacters:&chr length:1];

    // Make sure "shift" is not removed from modifiers below.
    eventModifiers |= NSFunctionKeyMask;
  }
  if ([[self keyEquivalent] characterAtIndex:0] == NSDeleteCharacter &&
      [eventString characterAtIndex:0] == NSDeleteFunctionKey) {
    unichar chr = NSDeleteCharacter;
    eventString = [NSString stringWithCharacters:&chr length:1];

    // Make sure "shift" is not removed from modifiers below.
    eventModifiers |= NSFunctionKeyMask;
  }

  // We intentionally leak this object.
  static __attribute__((unused)) KeyboardInputSourceListener* listener =
      [[KeyboardInputSourceListener alloc] init];

  // We typically want to compare [NSMenuItem keyEquivalent] against [NSEvent
  // charactersIgnoringModifiers]. There is a special keyboard layout "Dvorak -
  // QWERTY" which uses QWERTY-style shortcuts when the Command key is held
  // down. In this case, we want to use the keycode of the event rather than
  // looking at the characters.
  if (g_is_input_source_dvorak_qwerty) {
    ui::KeyboardCode windows_keycode =
        ui::KeyboardCodeFromKeyCode(event.keyCode);
    unichar shifted_character, character;
    ui::MacKeyCodeForWindowsKeyCode(windows_keycode, event.modifierFlags,
                                    &shifted_character, &character);
    eventString = [NSString stringWithFormat:@"%C", shifted_character];
  }

  // [ctr + shift + tab] generates the "End of Medium" keyEquivalent rather than
  // "Horizontal Tab". We still use "Horizontal Tab" in the main menu to match
  // the behavior of Safari and Terminal. Thus, we need to explicitly check for
  // this case.
  if ((eventModifiers & NSShiftKeyMask) &&
      [eventString isEqualToString:@"\x19"]) {
    eventString = @"\x9";
  } else {
    // Clear shift key for printable characters, excluding tab.
    if ((eventModifiers & (NSNumericPadKeyMask | NSFunctionKeyMask)) == 0 &&
        [[self keyEquivalent] characterAtIndex:0] != '\r' &&
        [[self keyEquivalent] characterAtIndex:0] != '\x9') {
      eventModifiers &= ~NSShiftKeyMask;
    }
  }

  // Clear all non-interesting modifiers
  eventModifiers &= NSCommandKeyMask |
                    NSControlKeyMask |
                    NSAlternateKeyMask |
                    NSShiftKeyMask;

  // On Czech keyboards, we want to interpret cmd + '+' as cmd + '1'.
  // htts://crbug.com/889424. We don't need special handling for other numeric
  // keys because they produce non-ASCII characters, and we already have logic
  // that ignores non-ASCII characters in favor of modified characters.
  if (g_is_input_source_czech) {
    if (eventModifiers == NSCommandKeyMask &&
        [eventString isEqualToString:@"+"]) {
      eventString = @"1";
    }
  }

  return [eventString isEqualToString:[self keyEquivalent]]
      && eventModifiers == [self keyEquivalentModifierMask];
}

@end
