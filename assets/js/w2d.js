/* document.addEventListener("click", (e) => { 
    var id = e.target.id; 
    if (id) openMenu(id); 
}, true); */

/** Invoke configure mozActivity/webActivity and redirect user to a Settings page
 * 
 * @author Tom Barrasso <tom@barrasso.me>
 * @author Luxferre
 * @author The Gaia Team at Mozilla and KaiOS Technologies
 * @param section - section ID for the page in the Settings app
 * @implements mozActivity (KaiOS 2.5)
 * @implements webActivity (KaiOS 3 and later)
 * @see https://gitlab.com/project-pris/system/-/blob/master/src/system/b2g/webapps/settings.gaiamobile.org/src/index.html?ref_type=heads
 * @see https://gitlab.com/project-pris/system/-/tree/master/src/system/b2g/webapps/settings.gaiamobile.org/src/elements?ref_type=heads
 */
function openMenu(section) {
    // KaiOS 2.5
    if (window.MozActivity) {
        var act = new MozActivity({
            name: "configure",
            data: {
                target: "device",
                section: section,
            },
        });
        act.onerror = function (e) {
            console.error(act, e);
            window.alert("Error:", JSON.stringify(act), e);
        };
    }
    // KaiOS 3 and later
    else if (window.WebActivity) {
        var act = new WebActivity("configure", {
            target: "device",
            section: section,
        });
        act.start().catch(function (e) {
            console.error(e, act);
            window.alert("Error: " + e);
        });
    }
    // Not a KaiOS device?
    else {
        window.alert('It appears your device doesn\'t support the mozActivity or webActivity API. For this function to work, please open this page on a KaiOS device.');
    }
};

/** Configure adbd port to 5555 for wireless ADB connection
 * 
 * @requires navigator.engmodeExtension
 * @requires navigator.jrdExtension
 * @requires navigator.kaiosExtension
 */
function wadb() {
    var masterExt = navigator.engmodeExtension || navigator.jrdExtension || navigator.kaiosExtension;
    var propSet = {
        'service.adb.tcp.port': 5555,
        'ctl.stop': 'adbd',
        'ctl.start': 'adbd'
    };
    for (var key in propSet) {
        masterExt.setPropertyValue(key, propSet[key])
    };
    window.alert('ADB port has been set to 5555.');
}