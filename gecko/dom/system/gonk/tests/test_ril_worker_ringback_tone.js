/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

subscriptLoader.loadSubScript("resource://gre/modules/ril_consts.js", this);

function run_test() {
  run_next_test();
}

add_test(function test_unsolicited_ringbacktone_change() {
  let workerHelper = newInterceptWorker();
  let worker  = workerHelper.worker;
  let context = worker.ContextPool._contexts[0];
  let buf     = context.Buf;

  buf.readInt32 = function fakeReadUint32() {
    return buf.int32Array.pop();
  };

  function do_test(state, expectedState) {
    buf.int32Array = [
        1,
        state  //stat/stop ringback tone.
    ];

    context.RIL[UNSOLICITED_RINGBACK_TONE]();

    let postedMessage = workerHelper.postedMessage;
    do_print("trigger ringbackTone");
    do_check_eq(postedMessage.rilMessageType, "ringbackTone");
    do_check_eq(postedMessage.playRingbackTone, expectedState);
  }

  let TEST_DATA = [
    // [state, expect]
    [1, true],
    [0, false],
  ];

  for (let i = 0; i < TEST_DATA.length; i++) {
    do_test.apply(null, TEST_DATA[i]);
  }

  run_next_test();
});