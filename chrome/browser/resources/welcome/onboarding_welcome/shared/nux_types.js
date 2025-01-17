// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

cr.exportPath('nux');

/**
 * @typedef {{
 *   id: number,
 *   name: string,
 *   icon: string,
 *   url: string,
 * }}
 */
nux.BookmarkListItem;

/**
 * @typedef {{
 *   total: number,
 *   active: number,
 * }}
 */
nux.stepIndicatorModel;