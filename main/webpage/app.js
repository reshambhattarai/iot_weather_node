/**
 * Add gobals here
 */
var seconds 	= null;
var otaTimerVar =  null;
var wifiConnectInterval = null;

/**
 * Initialize functions here.
 */
$(document).ready(function(){
	getUpdateStatus();
	startDHTSensorInterval();
	updateFirmware();
	getConnectInfo();
	$("#connect_wifi").on("click", function(){
		checkCredentials();
	});
	$("#disconnect_wifi").on("click", function(){
			disconnectWifi();
		});
});   

/**
 * Gets file name and size for display on the web page.
 */        
function getFileInfo() 
{
    var x = document.getElementById("selected_file");
    var file = x.files[0];

    document.getElementById("file_info").innerHTML = "<h4>File: " + file.name + "<br>" + "Size: " + file.size + " bytes</h4>";
}

/**
 * Handles the firmware update.
 */
function updateFirmware() 
{
    // Form Data
    var formData = new FormData();
    var fileSelect = document.getElementById("selected_file");
    
    if (fileSelect.files && fileSelect.files.length == 1) 
	{
        var file = fileSelect.files[0];
        formData.set("file", file, file.name);
        document.getElementById("ota_update_status").innerHTML = "Uploading " + file.name + ", Firmware Update in Progress...";

        // Http Request
        var request = new XMLHttpRequest();

        request.upload.addEventListener("progress", updateProgress);
        request.open('POST', "/OTAupdate");
        request.responseType = "blob";
        request.send(formData);
    } 
	else 
	{
        window.alert('Select A File First')
    }
}

/**
 * Progress on transfers from the server to the client (downloads).
 */
function updateProgress(oEvent)
{
    if (oEvent.lengthComputable) 
	{
        getUpdateStatus();
    } 
	else 
	{
        window.alert('total size is unknown')
    }
}

/**
 * Posts the firmware udpate status.
 */
function getUpdateStatus() 
{
    var xhr = new XMLHttpRequest();
    var requestURL = "/OTAstatus";
    xhr.open('POST', requestURL, false);
    xhr.send('ota_update_status');

    if (xhr.readyState == 4 && xhr.status == 200) 
	{		
        var response = JSON.parse(xhr.responseText);
						
	 	document.getElementById('latest_firmware').innerHTML = response.compile_date + " - " + response.compile_time

		// If flashing was complete it will return a 1, else -1
		// A return of 0 is just for information on the Latest Firmware request
        if (response.ota_update_status == 1) 
		{
    		// Set the countdown timer time
            seconds = 10;
            // Start the countdown timer
            otaRebootTimer();
        } 
        else if (response.ota_update_status == -1)
		{
            document.getElementById('ota_update_status').innerHTML = "!!! Upload Error !!!";
        }
    }
}

/**
 * Displays the reboot countdown.
 */
function otaRebootTimer() 
{	
    document.getElementById('ota_update_status').innerHTML = "OTA Firmware Update Complete. This page will close shortly, Rebooting in: " + seconds;

    if (--seconds == 0) 
	{
        clearTimeout(otaTimerVar);
        window.location.reload();
    } 
	else 
	{
        otaTimerVar = setTimeout(otaRebootTimer, 1000);
    }
}

/*
Get dht22 sensor reading and display on the webpage
*/
function getDHTSensorValues(){
	$.getJSON('/dhtSensor.json', function(data){
		$("#temperature_reading").text(data["temp"]);
		$("#humidity_reading").text(data["humidity"]);
	});
}

function startDHTSensorInterval(){
	setInterval(getDHTSensorValues, 5000);
}

/*
clears the connection status interval
*/

function stopWifiConnectStatusInterval(){
	if(wifiConnectInterval!=null){
		clearInterval(wifiConnectInterval);
		wifiConnectInterval = null;
	}
}

/*
gets wifi connection status
*/
function getWifiConnectStatus(){
	var xhr  = new XMLHttpRequest();
	var requestURL = "/wifiConnectStatus";
	xhr.open('POST', requestURL, false);
	xhr.send('wifi_connect_status');
	
	if(xhr.readyState == 4 && xhr.status==200)
		{
			var response  = JSON.parse(xhr.responseText);
			
			document.getElementById('wifi_connect_status').innerHTML = "Connecting...";
			
			if(response.wifi_connect_status == 2){
				document.getElementById('wifi_connect_status').innerHTML = "<h2 class= 'rd'> Failed to Connect. Please your Ap credentials and compatibility</h2>";
				stopWifiConnectStatusInterval();
			}
			else if(response.wifi_connect_status == 3)
				{
					document.getElementById('wifi_connect_status').innerHTML = "<h2 class= 'gr'> Connection Success</h2>";
					stopWifiConnectStatusInterval();
					getConnectInfo();
				}
		}
}

/**
 *starts the interval for checking the connection interval 
 */
function startWifiConnectStatusInterval(){
	wifiConnectInterval = setInterval(getWifiConnectStatus, 2800);
}

/**
 * connect wifi function called using th ssid and password entered into the textfields
 */
function connectWifi(){
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	$.ajax({
		url :'/wifiConnect.json',
		dataType:'json',
		method: 'POST',
		cache: false,
		headers: {'my-connect-ssid': selectedSSID, 'my-connect-pwd': pwd},
		data: {'timestamp': Date.now()}
	});
	startWifiConnectStatusInterval();
}

/**
 * checks credentials on connect_wifi button click
 */
function checkCredentials(){
	errorList = "";
	credsOk = true;
	selectedSSID = $("#connect_ssid").val();
	pwd = $("#connect_pass").val();
	
	if(selectedSSID == ""){
		errorList += "<h2 class='rd'>SSID cannont be empty</h2>";
		credsOk = false;
	}
	if(pwd == ""){
			errorList += "<h2 class='rd'>Password cannont be empty</h2>";
			credsOk = false;
		}
	if(credsOk==false){
		$("#wifi_connect_credentials_errors").html(errorList);
	}
	else{
		$("#wifi_connect_credentials_errors").html("");
		connectWifi();
	}
	
}


/**
 * shows wifi password if the show_password checkbox is true
 */

function showPassword(){
	var show  = document.getElementById("connect_pass");
	if(show.type =="password"){
		show.type="text";
	}
	else{
		show.type = "password";
	}
}

/**
 * gets information of connection for displaying on the web page
 */
function getConnectInfo(){
	$.getJSON('/wifiConnectInfo.json', function(data){
		$("#connected_ap_label").html("Connected to:");
		$("#connected_ap").text(data["ap"]);
		
		$("#ip_address_label").html("IP ADDRESS:");
		$("#wifi_connect_ip").text(data["ip"]);
		
		$("#netmask_label").html("NetMask:");
		$("#wifi_connect_netmask").text(data["netmask"]);
		
		$("#gateway_label").html("Gateway:");
		$("#wifi_connect_gw").text(data["gw"]);
		
		document.getElementById('disconnect_wifi').style.display = 'block';
	});
}


/**
 * disconnects wifi and reloads the page once disconnected
 */
function disconnectWifi(){
	$.ajax({
		url:'/wifiDisconnect.json',
		dataType: 'Json',
		method: 'DELETE',
		cache: false,
		data: {'timestamp': Date.now()}
	});
	//update the webpage
	setTimeout("location.reload(true)", 2000);
}