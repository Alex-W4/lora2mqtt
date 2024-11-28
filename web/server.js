const http = require('http');
const fs = require('fs');
const express = require('express');
const lora2mqtt = require('../lora/build/Release/lora2mqtt.node');
const {join} = require("node:path");


const app = express();

let client_list = [];

let discovered_devices = [];
let config;
let config_file = "config.json";
let config_path = "/etc/lora2mqtt";

MESSAGE_TYPE = {
    REQUEST: "request",
    DEVICES: "devices",
    DISCOVERY_LIST: "discovery_list",
    NEW_DEVICE: "new_device",
    REMOVE_DEVICE:"remove_device",
    MQTT_CONFIG: "mqtt_config",
    MQTT_STATUS: "mqtt_status",
    RESET_DISCOVERY_LIST: "reset_discovery_list",
    DELETE_DEVICE: "delete_device"
}

DEVICE_TYPE = {
    CISTERN: "Z"
}


try {
    if (!fs.existsSync(config_path)) {
        fs.mkdirSync(config_path);
    }
    if (fs.existsSync(config_path + "/" + config_file)) {
        const data = fs.readFileSync(config_path + "/" + config_file, 'utf8');
        config = JSON.parse(data);
    } else {
        config = create_default_conf();
        save_config();
    }
} catch (err) {
    console.error(err);
}

const server = http.createServer(app);
const WebSocket = require('ws').WebSocketServer;

const wss = new WebSocket({server: server});

let DEVICE = {
    WAG_ELEK_V1: 1,
}

//setInterval(update_discovered_devices, 5000);
//setInterval(send_devices, 2000);


wss.on('connection', function (ws) {
    console.log("New connection!")
    client_list.push({auth: false, ws: ws});
    ws.on('message', function (message) {
        //console.log('received %s', message);
        let msg_obj = JSON.parse(message);
        //console.log(msg_obj);

        switch (typeof msg_obj) {
            case "object":
                if (Object.hasOwn(msg_obj,"type")) {
                    switch (msg_obj.type) {
                        case MESSAGE_TYPE.NEW_DEVICE:
                            let data = msg_obj.payload;
                            //console.log(data);
                            if (Object.hasOwn(data,"mac") && Object.hasOwn(data,"name") && Object.hasOwn(data,"type") && Object.hasOwn(data,"version")) {
                                set_new_device(data.mac, data.name, data.type, data.version);
                            }
                            break
                        case MESSAGE_TYPE.DELETE_DEVICE:
                            let device_obj = msg_obj.payload;
                            if (Object.hasOwn(device_obj, "mac")) {
                                lora2mqtt.remove_from_device_list(device_obj.mac);
                                update_registered_devices();
                            }
                            break
                        case MESSAGE_TYPE.DISCOVERY_LIST:
                            discovered_devices = msg_obj.payload;
                            break
                        case MESSAGE_TYPE.MQTT_CONFIG:
                            //console.log("test mqtt setting");
                            let mqtt_conf = msg_obj.payload;
                            //console.log(mqtt_conf);
                            config.mqtt = mqtt_conf;
                            set_mqtt_config(mqtt_conf.host, mqtt_conf.port, mqtt_conf.username, mqtt_conf.password)
                            ws.send(JSON.stringify({type: MESSAGE_TYPE.MQTT_STATUS, payload: {status: "connected"}}));
                            update_mqtt_config();
                            update_mqtt_status()
                            break

                        case MESSAGE_TYPE.REQUEST:
                            if (Object.hasOwn(msg_obj.payload, "type")) {
                                switch (msg_obj.payload.type) {
                                    case MESSAGE_TYPE.DISCOVERY_LIST:
                                        update_discovered_devices();
                                        break
                                    case MESSAGE_TYPE.DEVICES:
                                        update_registered_devices();
                                        break
                                    case MESSAGE_TYPE.RESET_DISCOVERY_LIST:
                                        reset_discoverys();
                                        break
                                    case MESSAGE_TYPE.MQTT_CONFIG:
                                        update_mqtt_config();
                                        break
                                    case MESSAGE_TYPE.MQTT_STATUS:
                                        update_mqtt_status();
                                        break
                                }
                            }
                    }
                }
                break;
            case "string":
                break
        }
    })

    ws.on('close', function (ws) {
        client_list.splice(client_list.indexOf(ws), 1);
    })
})


/*
wss.on('connection', function connection(ws, request, client) {
    ws.on('error', console.error);

    ws.on('message', function message(data) {
        console.log(`Received message ${data} from user ${client}`);
    });
});

server.on('upgrade', function upgrade(request, socket, head) {
    socket.on('error', onSocketError);

    // This function is not defined on purpose. Implement it with your own logic.
    authenticate(request, function next(err, client) {
        if (err || !client) {
            socket.write('HTTP/1.1 401 Unauthorized\r\n\r\n');
            socket.destroy();
            return;
        }

        socket.removeListener('error', onSocketError);

        wss.handleUpgrade(request, socket, head, function done(ws) {
            wss.emit('connection', ws, request, client);
        });
    });
});*/



app.use(express.static(join(__dirname, 'public')));
server.listen(config.server.port);
console.log('Listening on port: ' + config.server.port);


//console.log(config.mqtt.host + "," + config.mqtt.port + "," + config.mqtt.username + "," + config.mqtt.password)
set_mqtt_config(config.mqtt.host, config.mqtt.port, config.mqtt.username, config.mqtt.password);
lora2mqtt.set_device(DEVICE.WAG_ELEK_V1);
//set_new_device("68:67:25:51:e6:58", "Zisterne Terasse", "Z", 1);
//lora2mqtt.set_mac_lookup("68:67:25:53:26:fc", "Zisterne Garage");

//lora2mqtt.call_callback();

function update_discovered_devices() {
    let macs = lora2mqtt.get_discovered_macs();
    discovered_devices = Array.from(macs.macs);
    send_discoverys();
    //console.log(discovered_devices);
    //lora2mqtt.reset_discovery_list();
}

function update_registered_devices() {
    let macs = lora2mqtt.get_registered_devices();
    config.devices = Array.from(macs.macs);
    send_devices();
    //console.log(config.devices);
    //lora2mqtt.reset_discovery_list();
}

function update_mqtt_config() {
    for (let client in client_list) {
        client_list[client].ws.send(JSON.stringify({type: MESSAGE_TYPE.MQTT_CONFIG, payload: config.mqtt}));
    }
}

function update_mqtt_status() {
    let mqtt_status = lora2mqtt.mqtt_is_connected();
    for (let client in client_list) {
        if (mqtt_status === 1) {
            client_list[client].ws.send(JSON.stringify({type: MESSAGE_TYPE.MQTT_STATUS, payload: {status: "connected"}}));
        } else {
            client_list[client].ws.send(JSON.stringify({type: MESSAGE_TYPE.MQTT_STATUS, payload: {status: "error"}}));
        }
    }
}

function set_new_device(mac, name, type, version) {
    lora2mqtt.set_mac_lookup(mac, name, type, version);
    config.devices.push({mac: mac, name: name, type: type, version: version});
    //console.log("index: " + discovered_devices.findIndex(obj => obj.mac === mac));
    //console.log("mac: " + mac + " length: " + mac.length);
    if (discovered_devices.length > 0) {
        //console.log("discovered_devices[0].mac" + discovered_devices[0].mac + " length: " + discovered_devices[0].mac.length);
    }
    lora2mqtt.remove_from_discovery_list(mac);
    discovered_devices.splice(discovered_devices.findIndex(obj => obj.mac === mac), 1)
    update_registered_devices();
}

function send_devices() {
    for (let client in client_list) {
        client_list[client].ws.send(JSON.stringify({type: "devices", payload: config.devices}));
    }
}

function send_discoverys() {
    for (let client in client_list) {
        client_list[client].ws.send(JSON.stringify({type: "discovery_list", payload: discovered_devices}));
    }
}

function reset_discoverys() {
    lora2mqtt.reset_discovery_list();
    update_discovered_devices();
}

function set_mqtt_config(host, port, username, password) {
    //console.log("" + host, "" + port, "" + username, "" + password)
    return lora2mqtt.set_mqtt_settings("" + host, "" + port, "" + username, "" + password);
}

function create_default_conf() {
    let config = {};
    config.devices = [];
    config.modules = [];
    config.mqtt = {};
    config.mqtt.host = "localhost";
    config.mqtt.port = 1883;
    config.mqtt.username = "";
    config.mqtt.password = "";
    config.server = {}
    config.server.port = 80;
    config.server.users = [];
    config.server.users.push({user: "admin", password: "admin"})
    return config;
}

function save_config() {
    try {
        fs.writeFileSync(config_path + "/" + config_file, JSON.stringify(config),'utf8');
    } catch (err) {
        console.error(err);
    }
}

