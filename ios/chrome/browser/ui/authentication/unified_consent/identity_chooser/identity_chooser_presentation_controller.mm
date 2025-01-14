// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/authentication/unified_consent/identity_chooser/identity_chooser_presentation_controller.h"

#import "ios/chrome/browser/ui/image_util/image_util.h"
#import "ios/chrome/browser/ui/util/uikit_ui_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kMaxWidth = 350;
const CGFloat kMaxHeight = 350;
const CGFloat kMinimumMarginHorizontal = 25;
const CGFloat kMinimumMarginVertical = 35;

const CGFloat kShadowMargin = 196;
const CGFloat kContainerCornerRadius = 13.0;
}  // namespace

@interface IdentityChooserPresentationController ()

@property(nonatomic, strong) UIView* shieldView;
@property(nonatomic, strong) UIView* shadowContainer;

@end

@implementation IdentityChooserPresentationController

@synthesize shieldView = _shieldView;
@synthesize shadowContainer = _shadowContainer;

#pragma mark - UIPresentationController

- (CGRect)frameOfPresentedViewInContainerView {
  CGRect safeAreaFrame = UIEdgeInsetsInsetRect(
      self.containerView.bounds, SafeAreaInsetsForView(self.containerView));

  CGFloat availableWidth = CGRectGetWidth(safeAreaFrame);
  CGFloat availableHeight = CGRectGetHeight(safeAreaFrame);

  CGFloat width = MIN(kMaxWidth, availableWidth - 2 * kMinimumMarginHorizontal);
  CGFloat height =
      MIN(kMaxHeight, availableHeight - 2 * kMinimumMarginVertical);

  CGRect presentedViewFrame = safeAreaFrame;
  presentedViewFrame.origin.x += (availableWidth - width) / 2;
  presentedViewFrame.origin.y += (availableHeight - height) / 2;
  presentedViewFrame.size.width = width;
  presentedViewFrame.size.height = height;

  return presentedViewFrame;
}

- (UIView*)presentedView {
  return self.shadowContainer;
}

- (void)presentationTransitionWillBegin {
  self.shieldView = [[UIView alloc] init];
  self.shieldView.frame = self.containerView.bounds;
  [self.containerView addSubview:self.shieldView];
  [self.shieldView
      addGestureRecognizer:[[UITapGestureRecognizer alloc]
                               initWithTarget:self
                                       action:@selector(handleShieldTap)]];

  self.shadowContainer = [[UIView alloc] init];

  UIView* contentClippingView = [[UIView alloc] init];
  contentClippingView.layer.cornerRadius = kContainerCornerRadius;
  contentClippingView.layer.masksToBounds = YES;
  contentClippingView.clipsToBounds = YES;

  UIImageView* shadowView =
      [[UIImageView alloc] initWithImage:StretchableImageNamed(@"menu_shadow")];

  shadowView.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  self.presentedViewController.view.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
  contentClippingView.autoresizingMask =
      UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

  [contentClippingView addSubview:self.presentedViewController.view];
  [self.shadowContainer addSubview:shadowView];
  [self.shadowContainer addSubview:contentClippingView];

  [self.containerView addSubview:self.shadowContainer];

  self.shadowContainer.frame = [self frameOfPresentedViewInContainerView];
  contentClippingView.frame = self.shadowContainer.bounds;
  self.presentedViewController.view.frame = self.shadowContainer.bounds;
  shadowView.frame =
      CGRectInset(self.shadowContainer.bounds, -kShadowMargin, -kShadowMargin);
}

- (void)containerViewWillLayoutSubviews {
  self.shieldView.frame = self.containerView.bounds;

  self.shadowContainer.frame = [self frameOfPresentedViewInContainerView];
}

#pragma mark - Private

- (void)handleShieldTap {
  [self.presentedViewController dismissViewControllerAnimated:YES
                                                   completion:nil];
}

@end
