var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig, null, { autoHandleEvents: false });

Pebble.addEventListener('showConfiguration', function() {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (!e.response) return;
  try {
    var settings = clay.getSettings(e.response, false);
    var msg = {
      inv:        settings.inv.value      ? 1 : 0,
      vibr:       settings.vibr.value     ? 1 : 0,
      datefmt:    settings.datefmt.value  ? 1 : 0,
      secs:       settings.secs.value     ? 1 : 0,
      vibr_bt:    settings.vibr_bt.value  ? 1 : 0,
      datemode:   settings.datemode.value ? 1 : 0,
      showsec:    settings.showsec.value,
      redsec:     settings.redsec.value   ? 1 : 0,
      secsrefresh: parseInt(settings.secsrefresh.value, 10) || 1,
      shakesecs:    settings.shakesecs.value    ? 1 : 0,
      shakesecsdur: parseInt(settings.shakesecsdur.value, 10) || 10
    };
    console.log('Sending settings: ' + JSON.stringify(msg));
    Pebble.sendAppMessage(msg,
      function()    { console.log('Settings sent OK'); },
      function(err) { console.log('Settings send failed: ' + JSON.stringify(err)); }
    );
  } catch(err) {
    console.log('Config error: ' + err);
  }
});
