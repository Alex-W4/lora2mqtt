let VIEW = {
    DEVICES: "device_list",
    NEW_DEVICE_SEARCH: "new_device_search",
    NEW_DEVICE_CONFIG: "new_device_config",
    DEVICE_DETAILS: "device_details",
    MQTT_DETAILS: "mqtt_details",
    MQTT_CONFIG: "mqtt_config",
    LORA_MODULES: "lora_module_list",
    LORA_MODULE_DETAILS: "lora_module_details",
    LORA_NEW_MODULE_LIST: "lora_new_module_list",
    LORA_NEW_MODULE_CONFIG: "lora_new_module_config",
    SETTINGS: "settings",
    SETTINGS_CONFIG: "settings_config"
}

let MESSAGE_TYPE = {
    REQUEST: "request",
    DEVICES: "devices",
    DISCOVERY_LIST: "discovery_list",
    NEW_DEVICE: "new_device",
    REMOVE_DEVICE: "remove_device",
    MQTT_CONFIG: "mqtt_config",
    MQTT_STATUS: "mqtt_status",
    RESET_DISCOVERY_LIST: "reset_discovery_list",
    DELETE_DEVICE: "delete_device"
}

DEVICE_TYPE = {
    CISTERN: "Z"
}

CISTERN_VERSION = {
    0: "1.0",
    1: "1.1"
}

let websocket_ready = false;
let device_list = [];
let discovery_list = [];
let current_view = VIEW.DEVICES;
let ws;
let discovery_interval_handle;
let mqtt_conf;


let check_mqtt_string = "teste Verbindung";
let animation_counter = 0;
let animation_interval_handle;

function websocket_connect(ws_host) {
    ws = new WebSocket(ws_host);

    ws.onopen = function (event) {
        console.log("WS open!");
        websocket_ready = true;
        request_devices();
        request_discovered_devices()
    };

    ws.onmessage = function (message) {
        //console.log(message);
        let msg_obj = JSON.parse(message.data);

        switch (typeof msg_obj) {
            case "object":
                if (Object.hasOwn(msg_obj, "type")) {
                    switch (msg_obj.type) {
                        case MESSAGE_TYPE.DEVICES:
                            device_list = msg_obj.payload;
                            if (current_view === VIEW.DEVICES) {
                                select_view(current_view)
                            }
                            break
                        case MESSAGE_TYPE.DISCOVERY_LIST:
                            discovery_list = msg_obj.payload;
                            if (current_view === VIEW.NEW_DEVICE_SEARCH) {
                                select_view(current_view)
                            }
                            break
                        case MESSAGE_TYPE.MQTT_CONFIG:
                            mqtt_conf = msg_obj.payload;
                            if (current_view === VIEW.MQTT_DETAILS) {
                                select_view(current_view)
                            }
                            break
                        case MESSAGE_TYPE.MQTT_STATUS:
                            let mqtt_status = msg_obj.payload;
                            if (animation_interval_handle !== undefined) {
                                clearInterval(animation_interval_handle);
                                animation_interval_handle = undefined;
                            }

                            if (Object.hasOwn(mqtt_status, "status")) {
                                if (mqtt_status.status === "connected") {
                                    if (current_view === VIEW.MQTT_DETAILS) {
                                        document.getElementById("mqtt_status_details").innerText = "Status: Verbunden";
                                    }
                                    if (current_view === VIEW.MQTT_CONFIG) {
                                        document.getElementById("mqtt_status_text").innerText = "Verbunden!";
                                    }
                                } else {
                                    if (current_view === VIEW.MQTT_DETAILS) {
                                        document.getElementById("mqtt_status_details").innerText = "Status: Getrennt";
                                    }
                                    if (current_view === VIEW.MQTT_CONFIG) {
                                        document.getElementById("mqtt_status_text").innerText = "Fehlgeschlagen!";
                                    }
                                }
                            } else {
                                document.getElementById("mqtt_status_details").innerText = "Status: Getrennt";
                                document.getElementById("mqtt_status_text").innerText = "Fehlgeschlagen!";
                            }
                    }
                }
                break;
            case "string":
                break
        }
    };

    ws.onclose = function () {

    };

    ws.onerror = function (ev) {
        console.log(ev);
        ws.close();
    }
}

function init() {
    websocket_connect("ws://" + location.hostname + "/ws");
    select_view(VIEW.DEVICES);
    setTimeout(request_devices, 500);
}

init();

function select_view(view, options) {
    if (discovery_interval_handle !== undefined) {
        clearInterval(discovery_interval_handle);
        discovery_interval_handle = undefined;
    }
    current_view = view;
    for (let box in VIEW) {
        document.getElementById(VIEW[box] + "_box").style.display = "none";
    }
    let insert_string = "";

    switch (view) {
        case VIEW.DEVICES:
            for (let entry in device_list) {
                insert_string = insert_string + generate_device_entry(device_list[entry]).outerHTML;
            }
            insert_string = insert_string + generate_add_device_entry().outerHTML;
            document.getElementById(VIEW.DEVICES + "_box").innerHTML = insert_string;
            document.getElementById(VIEW.DEVICES + "_box").style.removeProperty("display");
            break;
        case VIEW.NEW_DEVICE_SEARCH:
            discovery_interval_handle = setInterval(request_discovered_devices, 1000);
            for (let entry in discovery_list) {
                insert_string = insert_string + generate_discovered_device_entry(discovery_list[entry]).outerHTML;
            }
            insert_string = insert_string + generate_reset_discoverys().outerHTML;
            document.getElementById(VIEW.NEW_DEVICE_SEARCH + "_box").innerHTML = insert_string;
            document.getElementById(VIEW.NEW_DEVICE_SEARCH + "_box").style.removeProperty("display");
            break;
        case VIEW.NEW_DEVICE_CONFIG:
            if (device_list.find(obj => obj.mac === options) !== undefined) {
                document.getElementById(VIEW.NEW_DEVICE_CONFIG + "_box").innerHTML = generate_new_device_entry(device_list.find(obj => obj.mac === options)).outerHTML;
                document.getElementById("new_device_name_input").value = device_list.find(obj => obj.mac === options).name;
            } else {
                document.getElementById(VIEW.NEW_DEVICE_CONFIG + "_box").innerHTML = generate_new_device_entry(discovery_list.find(obj => obj.mac === options)).outerHTML;
                document.getElementById("new_device_name_input").value = "";
            }
            document.getElementById(VIEW.NEW_DEVICE_CONFIG + "_box").style.removeProperty("display");
            break;
        case VIEW.DEVICE_DETAILS:
            let lookup_entry = device_list.find(dev => dev.mac === options)
            document.getElementById("device_details_box").innerHTML = generate_details_entry(lookup_entry).outerHTML;
            document.getElementById(VIEW.DEVICE_DETAILS + "_box").style.removeProperty("display");
            break;
        case VIEW.MQTT_DETAILS:
            request_mqtt_status();
            if (mqtt_conf === undefined) {
                request_mqtt_config();
            } else {
                document.getElementById(VIEW.MQTT_DETAILS + "_box").innerHTML = generate_mqtt_entry(mqtt_conf).outerHTML;
                document.getElementById(VIEW.MQTT_DETAILS + "_box").style.removeProperty("display");
            }
            break;
        case VIEW.MQTT_CONFIG:
            document.getElementById(VIEW.MQTT_CONFIG + "_box").innerHTML = generate_change_mqtt().outerHTML;
            document.getElementById("mqtt_host_input").value = mqtt_conf.host;
            document.getElementById("mqtt_port_input").value = mqtt_conf.port;
            document.getElementById("mqtt_username_input").value = mqtt_conf.username;
            document.getElementById("mqtt_password_input").value = mqtt_conf.password;
            document.getElementById("mqtt_status_text").style.display = "none";
            document.getElementById(VIEW.MQTT_CONFIG + "_box").style.removeProperty("display");
            break;
        case VIEW.LORA_MODULES:
            document.getElementById(VIEW.LORA_MODULES + "_box").style.removeProperty("display");
            break;
        case VIEW.LORA_MODULE_DETAILS:
            document.getElementById(VIEW.LORA_MODULE_DETAILS + "_box").style.removeProperty("display");
            break;
        case VIEW.LORA_NEW_MODULE_LIST:
            document.getElementById(VIEW.LORA_NEW_MODULE_LIST + "_box").style.removeProperty("display");
            break;
        case VIEW.LORA_NEW_MODULE_CONFIG:
            document.getElementById(VIEW.LORA_NEW_MODULE_CONFIG + "_box").style.removeProperty("display");
            break;
        case VIEW.SETTINGS:
            document.getElementById(VIEW.SETTINGS + "_box").style.removeProperty("display");
            break;
        case VIEW.SETTINGS_CONFIG:
            document.getElementById(VIEW.SETTINGS_CONFIG + "_box").style.removeProperty("display");
            break;

    }
}

function to_discovery_list() {
    request_discovered_devices();
    select_view(VIEW.NEW_DEVICE_SEARCH);
}

function to_devices() {
    request_devices();
    select_view(VIEW.DEVICES);
}

function set_new_device(div_id) {
    let send_lookup = discovery_list.find(obj => obj.mac === div_id)
    if (send_lookup === undefined) {
        send_lookup = device_list.find(obj => obj.mac === div_id)
    }
    console.log(send_lookup);
    let send_mac = send_lookup.mac;
    let send_name = sanatize_string(document.getElementById("new_device_name_input").value);
    let send_type = send_lookup.device;
    let send_version = send_lookup.version;
    let send_obj = {
        type: MESSAGE_TYPE.NEW_DEVICE,
        payload: {mac: send_mac, name: send_name, type: send_type, version: send_version}
    };
    console.log(send_obj);
    ws.send(JSON.stringify(send_obj));
    select_view(VIEW.DEVICES);
    request_devices();
}

function delete_device(dev_mac) {
    ws.send(JSON.stringify({type: MESSAGE_TYPE.DELETE_DEVICE, payload: {mac: dev_mac}}));
    select_view(VIEW.DEVICES);
}

function sanatize_string(input) {
    return input;
}

function request_devices() {
    ws.send(JSON.stringify({type: MESSAGE_TYPE.REQUEST, payload: {type: MESSAGE_TYPE.DEVICES}}));
}

function request_discovered_devices() {
    ws.send(JSON.stringify({type: MESSAGE_TYPE.REQUEST, payload: {type: MESSAGE_TYPE.DISCOVERY_LIST}}));
}

function request_mqtt_config() {
    ws.send(JSON.stringify({type: MESSAGE_TYPE.REQUEST, payload: {type: MESSAGE_TYPE.MQTT_CONFIG}}));
}

function request_mqtt_status() {
    ws.send(JSON.stringify({type: MESSAGE_TYPE.REQUEST, payload: {type: MESSAGE_TYPE.MQTT_STATUS}}));
}

function reset_discovery_list() {
    ws.send(JSON.stringify({type: MESSAGE_TYPE.REQUEST, payload: {type: MESSAGE_TYPE.RESET_DISCOVERY_LIST}}));
}

function save_mqtt_settings() {
    let host = document.getElementById("mqtt_host_input").value;
    let port = document.getElementById("mqtt_port_input").value;
    let username = document.getElementById("mqtt_username_input").value;
    let password = document.getElementById("mqtt_password_input").value;
    document.getElementById("mqtt_host_input").disabled = true;
    document.getElementById("mqtt_port_input").disabled = true;
    document.getElementById("mqtt_username_input").disabled = true;
    document.getElementById("mqtt_password_input").disabled = true;
    document.getElementById("mqtt_save_button").style.display = "none";
    document.getElementById("mqtt_status_text").style.removeProperty("display");
    let send_conf = {host: host, port: port, username: username, password: password};
    ws.send(JSON.stringify({type: MESSAGE_TYPE.MQTT_CONFIG, payload: send_conf}));
    animation_interval_handle = setInterval(testing_animation, 500);
}

function testing_animation() {
    let input_string;
    switch (animation_counter) {
        case 0:
            input_string = check_mqtt_string + "."
            animation_counter = 1;
            break
        case 1:
            input_string = check_mqtt_string + ".."
            animation_counter = 2;
            break
        case 2:
            input_string = check_mqtt_string + "..."
            animation_counter = 0;
            break

    }
    document.getElementById("mqtt_status_text").innerText = input_string;
}

function generate_device_entry(device_lookup) {
    let dummy = document.createElement("div");
    dummy.classList.add("device_entry_dummy")

    let image = document.createElement("img");
    // switch statement
    image.src = "img/cistern.svg";
    // switch statement end
    image.classList.add("device_entry_img");
    let image_entry = document.createElement("div");
    image_entry.classList.add("device_entry_img");
    image_entry.appendChild(image);
    let image_box = document.createElement("div");
    image_box.classList.add("device_entry_img_box");
    image_box.appendChild(image_entry);

    let name = document.createElement("p");
    name.classList.add("device_entry_text");
    name.innerText = device_lookup.name;
    let name_box = document.createElement("div");
    name_box.classList.add("device_entry_text_box")
    name_box.appendChild(name);

    let entry_box = document.createElement("div");
    entry_box.classList.add("device_entry_box");
    entry_box.classList.add("light_hover");
    entry_box.id = device_lookup.mac;
    entry_box.setAttribute("onclick", "select_view('" + VIEW.DEVICE_DETAILS + "', '" + device_lookup.mac + "')");
    entry_box.appendChild(image_box);
    entry_box.appendChild(name_box);

    let device_entry = document.createElement("div");
    device_entry.classList.add("device_entry");
    device_entry.appendChild(dummy);
    device_entry.appendChild(entry_box);
    return device_entry;
}

function generate_add_device_entry() {
    let dummy = document.createElement("div");
    dummy.classList.add("device_entry_dummy")

    let image = document.createElement("img");
    image.src = "img/cross.svg";
    image.classList.add("device_entry_img_add");
    let image_entry = document.createElement("div");
    image_entry.classList.add("device_entry_img_add");
    image_entry.appendChild(image);
    let image_box = document.createElement("div");
    image_box.classList.add("device_entry_img_add_box");
    image_box.appendChild(image_entry);

    let entry_box = document.createElement("div");
    entry_box.classList.add("device_entry_box");
    entry_box.classList.add("light_hover");
    entry_box.id = "device_entry_add";
    entry_box.setAttribute("onclick", "to_discovery_list()");
    entry_box.appendChild(image_box);

    let device_entry = document.createElement("div");
    device_entry.classList.add("device_entry");
    device_entry.appendChild(dummy);
    device_entry.appendChild(entry_box);
    return device_entry;
}

function generate_discovered_device_entry(discovery_lookup) {
    //discovery_lookup.device;
    let dummy = document.createElement("div");
    dummy.classList.add("device_entry_dummy")

    let image = document.createElement("img");
    // switch statement
    image.src = "img/cistern.svg";
    // switch statement end
    image.classList.add("device_entry_img");
    let image_entry = document.createElement("div");
    image_entry.classList.add("device_entry_img");
    image_entry.appendChild(image);
    let image_box = document.createElement("div");
    image_box.classList.add("device_entry_img_box");
    image_box.appendChild(image_entry);

    let name = document.createElement("p");
    name.classList.add("device_entry_text");
    name.innerText = discovery_lookup.mac + "  Version: " + CISTERN_VERSION[discovery_lookup.version];
    let name_box = document.createElement("div");
    name_box.classList.add("device_entry_text_box")
    name_box.appendChild(name);

    let entry_box = document.createElement("div");
    entry_box.classList.add("device_entry_box");
    entry_box.classList.add("light_hover");
    entry_box.id = discovery_lookup.mac;
    entry_box.setAttribute("onclick", "select_view('" + VIEW.NEW_DEVICE_CONFIG + "', '" + discovery_lookup.mac + "')");
    entry_box.appendChild(image_box);
    entry_box.appendChild(name_box);

    let device_entry = document.createElement("div");
    device_entry.classList.add("device_entry");
    device_entry.appendChild(dummy);
    device_entry.appendChild(entry_box);
    return device_entry;
}

function generate_details_entry(device_lookup) {

    let img = document.createElement("img");
    img.classList.add("device_details_img");
    // switch statement
    img.src = "img/cistern.svg";
    // switch statement end
    img.id = "details_img";
    let img_box = document.createElement("div");
    img_box.classList.add("device_details_img_box");
    img_box.appendChild(img);


    let details_info_box = document.createElement("div");
    details_info_box.classList.add("device_details_info_box");
    // switch statement
    let details_info_device = document.createElement("div");
    details_info_device.classList.add("device_details_info_entry");
    let details_info_device_p = document.createElement("p");
    details_info_device_p.innerText = "Gerät: " + "Zisternensensor";
    details_info_device.appendChild(details_info_device_p);
    details_info_box.appendChild(details_info_device);

    let details_info_version = document.createElement("div");
    details_info_version.classList.add("device_details_info_entry");
    let details_info_version_p = document.createElement("p");
    details_info_version_p.innerText = "Version: " + CISTERN_VERSION[device_lookup.version];
    details_info_version.appendChild(details_info_version_p);
    details_info_box.appendChild(details_info_version);

    let details_info_mac = document.createElement("div");
    details_info_mac.classList.add("device_details_info_entry");
    let details_info_mac_p = document.createElement("p");
    details_info_mac_p.innerText = "MAC: " + device_lookup.mac;
    details_info_mac.appendChild(details_info_mac_p);
    details_info_box.appendChild(details_info_mac);

    let details_info_name = document.createElement("div");
    details_info_name.classList.add("device_details_info_entry");
    let details_info_name_p = document.createElement("p");
    details_info_name_p.innerText = "Name für MQTT topic: " + device_lookup.name;
    details_info_name.appendChild(details_info_name_p);
    details_info_box.appendChild(details_info_name);
    // switch statement end

    let edit_button = document.createElement("div");
    edit_button.classList.add("new_device_button");
    edit_button.classList.add("light_hover");
    edit_button.innerText = "bearbeiten";
    edit_button.setAttribute("onclick", "select_view('" + VIEW.NEW_DEVICE_CONFIG + "','" + device_lookup.mac + "')");
    details_info_box.appendChild(edit_button);

    let delete_button = document.createElement("div");
    delete_button.classList.add("new_device_button");
    delete_button.classList.add("light_hover");
    delete_button.innerText = "löschen";
    delete_button.setAttribute("onclick", "delete_device('" + device_lookup.mac + "')");
    details_info_box.appendChild(delete_button);

    let details_box = document.createElement("div");
    details_box.classList.add("device_details_box");
    details_box.appendChild(img_box);
    details_box.appendChild(details_info_box);

    let entry_box = document.createElement("div");
    entry_box.classList.add("device_details_entry");
    entry_box.appendChild(details_box);
    return entry_box;
}

function generate_new_device_entry(discovery_lookup) {

    let img = document.createElement("img");
    img.classList.add("device_details_img");
    // switch statement
    img.src = "img/cistern.svg";
    // switch statement end
    img.id = "new_device_img";
    let img_box = document.createElement("div");
    img_box.classList.add("device_details_img_box");
    img_box.appendChild(img);


    let details_info_box = document.createElement("div");
    details_info_box.classList.add("device_details_info_box");
    // switch statement
    let details_info_device = document.createElement("div");
    details_info_device.classList.add("device_details_info_entry");
    let details_info_device_p = document.createElement("p");
    details_info_device_p.innerText = "Gerät: " + "Zisternensensor";
    details_info_device.appendChild(details_info_device_p);
    details_info_box.appendChild(details_info_device);

    let details_info_version = document.createElement("div");
    details_info_version.classList.add("device_details_info_entry");
    let details_info_version_p = document.createElement("p");
    details_info_version_p.innerText = "Version: " + CISTERN_VERSION[discovery_lookup.version];
    details_info_version.appendChild(details_info_version_p);
    details_info_box.appendChild(details_info_version);

    let details_info_mac = document.createElement("div");
    details_info_mac.classList.add("device_details_info_entry");
    let details_info_mac_p = document.createElement("p");
    details_info_mac_p.innerText = "MAC: " + discovery_lookup.mac;
    details_info_mac.appendChild(details_info_mac_p);
    details_info_box.appendChild(details_info_mac);

    let details_info_name = document.createElement("div");
    details_info_name.classList.add("device_details_info_entry");
    let details_info_name_p = document.createElement("p");
    details_info_name_p.innerText = "Name für MQTT topic: ";
    let details_name_input = document.createElement("input");
    details_name_input.id = "new_device_name_input";
    details_name_input.type = "text";
    details_info_name_p.appendChild(details_name_input);
    details_info_name.appendChild(details_info_name_p);
    details_info_box.appendChild(details_info_name);

    // switch statement end
    let new_device_save_button = document.createElement("div");
    new_device_save_button.classList.add("new_device_button");
    new_device_save_button.classList.add("light_hover");
    new_device_save_button.innerText = "speichern";
    new_device_save_button.setAttribute("onclick", "set_new_device('" + discovery_lookup.mac + "')");
    details_info_box.appendChild(new_device_save_button);

    let details_box = document.createElement("div");
    details_box.classList.add("device_details_box");
    details_box.appendChild(img_box);
    details_box.appendChild(details_info_box);

    let entry_box = document.createElement("div");
    entry_box.classList.add("device_details_entry");
    entry_box.appendChild(details_box);
    return entry_box;
}

function generate_reset_discoverys() {
    let dummy = document.createElement("div");
    dummy.classList.add("device_entry_dummy")

    let image = document.createElement("img");
    image.src = "img/reset.svg";
    image.classList.add("device_entry_img_add");
    let image_entry = document.createElement("div");
    image_entry.classList.add("device_entry_img_add");
    image_entry.appendChild(image);
    let image_box = document.createElement("div");
    image_box.classList.add("device_entry_img_add_box");
    image_box.appendChild(image_entry);

    let entry_box = document.createElement("div");
    entry_box.classList.add("device_entry_box");
    entry_box.classList.add("light_hover");
    entry_box.id = "device_entry_add";
    entry_box.setAttribute("onclick", "reset_discovery_list()");
    entry_box.appendChild(image_box);

    let device_entry = document.createElement("div");
    device_entry.classList.add("device_entry");
    device_entry.appendChild(dummy);
    device_entry.appendChild(entry_box);
    return device_entry;
}

function generate_mqtt_entry(mqtt_config) {

    let img = document.createElement("img");
    img.classList.add("device_details_img");
    // switch statement
    img.src = "img/mqtt_logo.png";
    // switch statement end
    img.id = "mqtt_img";
    let img_box = document.createElement("div");
    img_box.classList.add("device_details_img_box");
    img_box.appendChild(img);


    let details_info_box = document.createElement("div");
    details_info_box.classList.add("device_details_info_box");
    // switch statement
    let details_info_device = document.createElement("div");
    details_info_device.classList.add("device_details_info_entry");
    let details_info_device_p = document.createElement("p");
    details_info_device_p.innerText = "Server: " + mqtt_config.host;
    details_info_device.appendChild(details_info_device_p);
    details_info_box.appendChild(details_info_device);

    let details_info_version = document.createElement("div");
    details_info_version.classList.add("device_details_info_entry");
    let details_info_version_p = document.createElement("p");
    details_info_version_p.innerText = "Port: " + mqtt_config.port;
    details_info_version.appendChild(details_info_version_p);
    details_info_box.appendChild(details_info_version);

    let details_info_mac = document.createElement("div");
    details_info_mac.classList.add("device_details_info_entry");
    let details_info_mac_p = document.createElement("p");
    details_info_mac_p.innerText = "Username (anonym wenn leer): " + mqtt_config.username;
    details_info_mac.appendChild(details_info_mac_p);
    details_info_box.appendChild(details_info_mac);

    let details_info_name = document.createElement("div");
    details_info_name.classList.add("device_details_info_entry");
    let details_info_name_p = document.createElement("p");
    details_info_name_p.innerText = "Passwort (ohne wenn leer): " + mqtt_config.password;
    details_info_name.appendChild(details_info_name_p);
    details_info_box.appendChild(details_info_name);

    let details_info_status = document.createElement("div");
    details_info_status.classList.add("device_details_info_entry");
    let details_info_status_p = document.createElement("p");
    details_info_status_p.innerText = "Status: ...";
    details_info_status_p.id = "mqtt_status_details";
    details_info_status.appendChild(details_info_status_p);
    details_info_box.appendChild(details_info_status);

    let edit_button = document.createElement("div");
    edit_button.classList.add("new_device_button");
    edit_button.classList.add("light_hover");
    edit_button.innerText = "bearbeiten";
    edit_button.setAttribute("onclick", "select_view('" + VIEW.MQTT_CONFIG + "')");
    details_info_box.appendChild(edit_button);

    let details_box = document.createElement("div");
    details_box.classList.add("device_details_box");
    details_box.appendChild(img_box);
    details_box.appendChild(details_info_box);

    let entry_box = document.createElement("div");
    entry_box.classList.add("device_details_entry");
    entry_box.appendChild(details_box);
    return entry_box;
}

function generate_change_mqtt() {

    let img = document.createElement("img");
    img.classList.add("device_details_img");
    // switch statement
    img.src = "img/mqtt_logo.png";
    // switch statement end
    img.id = "mqtt_img";
    let img_box = document.createElement("div");
    img_box.classList.add("device_details_img_box");
    img_box.appendChild(img);


    let details_info_box = document.createElement("div");
    details_info_box.classList.add("device_details_info_box");

    let details_info_device = document.createElement("div");
    details_info_device.classList.add("device_details_info_entry");
    let details_info_device_p = document.createElement("p");
    details_info_device_p.innerText = "Server: ";
    details_info_device.appendChild(details_info_device_p);
    let mqtt_host_input = document.createElement("input");
    mqtt_host_input.id = "mqtt_host_input";
    mqtt_host_input.type = "text";
    details_info_device.appendChild(mqtt_host_input);
    details_info_box.appendChild(details_info_device);

    let details_info_version = document.createElement("div");
    details_info_version.classList.add("device_details_info_entry");
    let details_info_version_p = document.createElement("p");
    details_info_version_p.innerText = "Port: ";
    details_info_version.appendChild(details_info_version_p);
    let mqtt_port_input = document.createElement("input");
    mqtt_port_input.id = "mqtt_port_input";
    mqtt_port_input.type = "text";
    details_info_version.appendChild(mqtt_port_input);
    details_info_box.appendChild(details_info_version);

    let details_info_mac = document.createElement("div");
    details_info_mac.classList.add("device_details_info_entry");
    let details_info_mac_p = document.createElement("p");
    details_info_mac_p.innerText = "Username (anonym wenn leer): ";
    details_info_mac.appendChild(details_info_mac_p);
    let mqtt_username_input = document.createElement("input");
    mqtt_username_input.id = "mqtt_username_input";
    mqtt_username_input.type = "text";
    details_info_mac.appendChild(mqtt_username_input);
    details_info_box.appendChild(details_info_mac);

    let details_info_name = document.createElement("div");
    details_info_name.classList.add("device_details_info_entry");
    let details_info_name_p = document.createElement("p");
    details_info_name_p.innerText = "Passwort (ohne wenn leer): ";
    details_info_name.appendChild(details_info_name_p);
    let mqtt_password_input = document.createElement("input");
    mqtt_password_input.id = "mqtt_password_input";
    mqtt_password_input.type = "text";
    details_info_name.appendChild(mqtt_password_input);
    details_info_box.appendChild(details_info_name);

    let edit_button = document.createElement("div");
    edit_button.classList.add("new_device_button");
    edit_button.classList.add("light_hover");
    edit_button.innerText = "speichern";
    edit_button.id = "mqtt_save_button";
    edit_button.setAttribute("onclick", "save_mqtt_settings()");
    details_info_box.appendChild(edit_button);

    let edit_status = document.createElement("div");
    edit_status.classList.add("new_device_button");
    edit_status.innerText = check_mqtt_string;
    edit_status.id = "mqtt_status_text";
    details_info_box.appendChild(edit_status);

    let details_box = document.createElement("div");
    details_box.classList.add("device_details_box");
    details_box.appendChild(img_box);
    details_box.appendChild(details_info_box);

    let entry_box = document.createElement("div");
    entry_box.classList.add("device_details_entry");
    entry_box.appendChild(details_box);
    return entry_box;
}
