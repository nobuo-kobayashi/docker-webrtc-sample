'use strict'

const port = process.env.PORT || 9449
const htmlDir = '/opt/data/html'

let connections = []
let index = 0

const express = require('express')
const app = express()
const expressWs = require('express-ws')(app);

app.ws('/', function(ws, req) {  
  let connectionId = 'conn_' + (index++)
  connections[connectionId] = ws

  console.log('ws connect...');

  // 自分以外の全員にメッセージを送信
  function _send(message) {
    for (let key in connections) {
      if (key != connectionId) {
        try {
          let _ws = connections[key]
          if (_ws) {
            _ws.send(message)
          }
        } catch (e) {
          console.log('websocket.send() error.', e)
        }
      }
    }
  }

  ws.on('message', function(message) {
    _send(message)
  });

  ws.on('close', function() {
    _send("playerDisconnected")
    delete connections[connectionId]
  });

  _send("playerConnected")
});

app.use(express.static(htmlDir))
app.listen(port, () => console.log('Listening on port ' + port + '...'))
