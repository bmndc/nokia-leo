/* (c) 2017 KAI OS TECHNOLOGIES (HONG KONG) LIMITED All rights reserved. This
 * file or any portion thereof may not be reproduced or used in any manner
 * whatsoever without the express written permission of KAI OS TECHNOLOGIES
 * (HONG KONG) LIMITED. KaiOS is the trademark of KAI OS TECHNOLOGIES (HONG KONG)
 * LIMITED or its affiliate company and may be registered in some jurisdictions.
 * All other trademarks are the property of their respective owners.
 */

/* global DEBUG, DEBUG_WORKER */
/* global REQUEST_SIM_AUTHENTICATION */

"use strict";

/**
 * Differ to TelephonyRequestQueue, SimAuthRequestQueue process request one after another.
 * That means, requests (push) will be pended before previous request result is back (pop).
 */
(function(exports) {
  const SIM_AUTH_REQUEST = [
    REQUEST_SIM_AUTHENTICATION
  ];

  // Set to true in ril_consts.js to see debug messages
  let DEBUG = DEBUG_WORKER;

  /**
   * Queue entry; only used in the queue.
   */
  let AuthRequestEntry = function(request, callback, token) {
    this.request = request;
    this.callback = callback;
    this.token = token;
  };

  let SimAuthRequestQueue = function(ril) {
    this.ril = ril;
    this.requestQueue = [];
    this.token = 0;
    this.processed = false;
  };

  SimAuthRequestQueue.prototype.push = function(request, callback) {
    if (!this.isValidRequest(request)) {
      debug("invalid request " + request);
      return;
    }

    if (this.token < Number.MAX_SAFE_INTEGER) {
      this.token++;
    } else {
      this.token = 1;
    }

    let entry = new AuthRequestEntry(request, callback, this.token);
    this.requestQueue.push(entry);
    this._executeEntry(entry);
  };

  /**
   * Currently, request queue only contain 1 type of command.
   * May extends to multiple type in the future.
   */
  SimAuthRequestQueue.prototype.pop = function(request) {
    if (this.requestQueue.length === 0) {
        this.debug("pop with request 0 length queue!");
      return;
    }

    let entry = this.requestQueue.shift();
    if (!entry.processed) {
      this.debug("the 1st item been pop has not been processed!!");
      return;
    }

    this._startQueue();
  };

  SimAuthRequestQueue.prototype._startQueue = function() {
    if (this.requestQueue.length === 0) {
      this.debug("start queue but queue is empty");
      return;
    }

    let entry = this.requestQueue[0];

    this._executeEntry(entry);
  };

  SimAuthRequestQueue.prototype._executeEntry = function(entry) {
    if (!entry) {
      this.debug("executeEntry without entry");
      return;
    }

    if (entry.processed) {
      this.debug("entry has been processed!");
      return;
    }

    if (this.requestQueue[0] !== entry) {
      this.debug("executeEntry but not the 1st item of queue");
      return;
    }

    entry.processed = true;
    entry.callback();
  };

  SimAuthRequestQueue.prototype.isValidRequest = function(request) {
    return SIM_AUTH_REQUEST.indexOf(request) !== -1;
  };

  SimAuthRequestQueue.prototype.debug = function(msg) {
    this.ril.context.debug("[SimAuthQ] " + msg);
  };

  // Before we make sure to form it as a module would not add extra
  // overhead of module loading, we need to define it in this way
  // rather than 'module.exports' it as a module component.
  exports.SimAuthRequestQueue = SimAuthRequestQueue;
})(self); // in worker self is the global