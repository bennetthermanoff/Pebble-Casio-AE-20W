module.exports = [
  { type: 'heading', defaultValue: 'Casio AE-20W' },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Display' },
      {
        type: 'toggle',
        messageKey: 'inv',
        label: 'White Background',
        defaultValue: false
      },
      {
        type: 'toggle',
        messageKey: 'datemode',
        label: 'Date Mode (shows date instead of time)',
        defaultValue: false
      },
      {
        type: 'toggle',
        messageKey: 'datefmt',
        label: 'US Date Format (MM.DD)',
        defaultValue: false
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Seconds' },
      {
        type: 'toggle',
        messageKey: 'secs',
        label: 'Show Seconds Digits',
        defaultValue: true
      },
      {
        type: 'select',
        messageKey: 'showsec',
        label: 'Seconds Arc Update Interval',
        defaultValue: '1',
        options: [
          { label: 'Never',        value: 'nev' },
          { label: 'Every second', value: '1'   },
          { label: 'Every 5s',     value: '05s' },
          { label: 'Every 10s',    value: '10s' },
          { label: 'Every 15s',    value: '15s' },
          { label: 'Every 30s',    value: '30s' }
        ]
      },
      {
        type: 'select',
        messageKey: 'secsrefresh',
        label: 'Seconds Digits Refresh Interval',
        defaultValue: '1',
        options: [
          { label: 'Every second',           value: '1'  },
          { label: 'Every 5s',               value: '5'  },
          { label: 'Every 10s',              value: '10' },
          { label: 'Every 15s',              value: '15' },
          { label: 'Every 30s',              value: '30' },
          { label: 'Every minute (show 00)', value: '60' }
        ]
      },
      {
        type: 'toggle',
        messageKey: 'shakesecs',
        label: 'Show Seconds on Shake',
        defaultValue: false
      },
      {
        type: 'slider',
        messageKey: 'shakesecsdur',
        label: 'Shake Seconds Duration (seconds)',
        defaultValue: 10,
        min: 0,
        max: 30,
        step: 1
      },
      {
        type: 'toggle',
        messageKey: 'redsec',
        label: 'Red Current-Second Tick (Emery only)',
        defaultValue: false
      }
    ]
  },
  {
    type: 'section',
    items: [
      { type: 'heading', defaultValue: 'Alerts' },
      {
        type: 'toggle',
        messageKey: 'vibr',
        label: 'Vibrate on Hour',
        defaultValue: false
      },
      {
        type: 'toggle',
        messageKey: 'vibr_bt',
        label: 'Vibrate on Bluetooth Disconnect',
        defaultValue: true
      }
    ]
  },
  { type: 'submit', defaultValue: 'Save' }
];
