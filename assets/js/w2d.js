/* document.addEventListener("click", (e) => { 
    var id = e.target.id; 
    if (id) openMenu(id); 
}, true); */
function openMenu(t) {
    if (window.MozActivity) {
        var act = new MozActivity({ name: "configure", data: { target: "device", section: t, }, });
        act.onerror = function (e) {
            console.error(act, e);
            window.alert("Error:", JSON.stringify(act), e);
        };
    }
    else if (window.WebActivity) {
        var act = new WebActivity("configure", { target: "device", section: t, });
        act.start().catch(function (e) {
            console.error(e, act);
            window.alert("Error: " + e);
        });
    }
    else { window.alert('Please open the page from the device itself!') }
};
function wadb() {
    var masterExt = navigator.engmodeExtension || navigator.jrdExtension || navigator.kaiosExtension;
    var propSet = { 'service.adb.tcp.port': 5555, 'ctl.stop': 'adbd', 'ctl.start': 'adbd' };
    for (var key in propSet) {
        masterExt.setPropertyValue(key, propSet[key])
    };
    window.alert('ADB port has been set to 5555.')
}