// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_VIEW_PUBLIC_CWV_NAVIGATION_DELEGATE_H_
#define IOS_WEB_VIEW_PUBLIC_CWV_NAVIGATION_DELEGATE_H_

#import <Foundation/Foundation.h>

#import "cwv_navigation_type.h"

@protocol CRIWVTranslateDelegate;
@class CWVSSLStatus;
@class CWVWebView;

// The decision to pass back to the decision handler from
// -webView:didFailNavigationWithSSLError:overridable:decisionHandler:.
typedef NS_ENUM(NSInteger, CWVSSLErrorDecision) {
  // Leave the failure as is and take no further action.
  CWVSSLErrorDecisionDoNothing = 0,
  // Ignore the error and reload the page.
  CWVSSLErrorDecisionOverrideErrorAndReload,
};

// Navigation delegate protocol for CWVWebViews.  Allows embedders to hook
// page loading and receive events for navigation.
@protocol CWVNavigationDelegate<NSObject>
@optional

// Asks delegate if WebView should start the load. WebView will
// load the request if this method is not implemented.
- (BOOL)webView:(CWVWebView*)webView
    shouldStartLoadWithRequest:(NSURLRequest*)request
                navigationType:(CWVNavigationType)navigationType;

// Asks delegate if WebView should continue the load. WebView
// will load the response if this method is not implemented.
// |forMainFrame| indicates whether the frame being navigated is the main frame.
- (BOOL)webView:(CWVWebView*)webView
    shouldContinueLoadWithResponse:(NSURLResponse*)response
                      forMainFrame:(BOOL)forMainFrame;

// Notifies the delegate that main frame navigation has started.
- (void)webViewDidStartProvisionalNavigation:(CWVWebView*)webView;

// Notifies the delegate that response data started arriving for
// the main frame.
- (void)webViewDidCommitNavigation:(CWVWebView*)webView;

// Notifies the delegate that page load has succeeded.
- (void)webViewDidFinishNavigation:(CWVWebView*)webView;

// Notifies the delegate that page load has failed.
// When the page load has failed due to an SSL certification error,
// -webView:didFailNavigationWithSSLError:overridable:decisionHandler:
// is called instead of this method.
- (void)webView:(CWVWebView*)webView didFailNavigationWithError:(NSError*)error;

// Notifies the delegate that the page load has failed due to an SSL error. If
// |overridable| is YES, the method can ignore the error and reload the page by
// calling |decisionHandler| with CWVSSLErrorDecisionOverrideErrorAndReload. The
// method can leave the failure as is by calling |decisionHandler| with
// CWVSSLErrorDecisionDoNothing.
//
// Note: When |decisionHandler| is called with
// CWVSSLErrorDecisionOverrideErrorAndReload, it must not be called
// synchronously in the method. It breaks status management and causes an
// assertion failure. It must be called asynchronously to avoid it.
- (void)webView:(CWVWebView*)webView
    didFailNavigationWithSSLError:(NSError*)error
                      overridable:(BOOL)overridable
                  decisionHandler:
                      (void (^)(CWVSSLErrorDecision))decisionHandler;

// Notifies the delegate that web view process was terminated
// (usually by crashing, though possibly by other means).
- (void)webViewWebContentProcessDidTerminate:(CWVWebView*)webView;

@end

#endif  // IOS_WEB_VIEW_PUBLIC_CWV_NAVIGATION_DELEGATE_H_
