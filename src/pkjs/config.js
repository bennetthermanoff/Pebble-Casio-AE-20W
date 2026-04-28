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
