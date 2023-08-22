"use strict";

this.EXPORTED_SYMBOLS = ["ProjectDisableApps"];

const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "libcutils", "resource://gre/modules/systemlibs.js");

const allApps = ['crossyroad', 'dangerdash', 'dangerdash-premium', 'doodlejump', 'englishwithoxford', 'nitrostreetrun', 'nitrostreetrun-premium', 'racingroad', 'realfootballrunner', 'realfootballrunner-premium', 'siberianstrike', 'siberianstrike-premium', 'snake', 'snake-premium'];

const leoApps = ['crossyroad', 'dangerdash', 'doodlejump', 'englishwithoxford', 'nitrostreetrun', 'racingroad', 'realfootballrunner', 'siberianstrike', 'snake'];

const sparklerApps = ['crossyroad', 'dangerdash-premium', 'doodlejump', 'englishwithoxford', 'nitrostreetrun-premium', 'racingroad', 'realfootballrunner-premium', 'siberianstrike-premium', 'snake-premium'];

const AMXApps = ['nitrostreetrun', 'realfootballrunner', 'siberianstrike', 'snake'];

function disableAppsList(){

  var list = [];
  const projectName = libcutils.property_get('ro.boot.project.name');

  dump("projectName="+projectName);

  if('Leo' === projectName){

    const skuID = libcutils.property_get('ro.boot.skuid');

    if((skuID === '62GMX') || (skuID === '62HMX')){
      list = getDisableApps(AMXApps);
      dump("AMX DisableApps");
    }else{
      list = getDisableApps(leoApps);
      dump("Leo DisableApps");
    }
  }else if ('Sparkler' === projectName){
    list = getDisableApps(sparklerApps);
    dump("Sparkler DisableApps");
  }

  dump("projectdisableAppsList :" + list);

  return list;
}

function getDisableApps(apps){
  for(var i=0;i<apps.length;i++){
    for(var j=0; j< allApps.length; j++){
      if(allApps[j] === apps[i]){
        allApps.splice(j, 1);
        j--;
      }
    }
  }
  return allApps;
}

// Public API
this.ProjectDisableApps = {
  disableAppsList: disableAppsList
};
