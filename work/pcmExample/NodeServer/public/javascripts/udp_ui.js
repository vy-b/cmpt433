"use strict";
// Client-side interactions with the browser.

// Make connection to server when web page is fully loaded.
var socket = io.connect();
$(document).ready(function() {
	sendCommandViaUDP("getvolume");
	sendCommandViaUDP("gettempo");

	$('#modeNone').click(function(){
		sendCommandViaUDP("modeNone");
	});
	$('#modeRock').click(function(){
		sendCommandViaUDP("modeRock");
	});
	$('#modeCustom').click(function(){
		sendCommandViaUDP("modeCustom");
	});
	$('#volumeDown').click(function(){
		sendCommandViaUDP("volumeDown");
	});
	$('#volumeUp').click(function(){
		sendCommandViaUDP("volumeUp");
	});
	$('#tempoUp').click(function(){
		sendCommandViaUDP("tempoUp");
	});
	$('#tempoDown').click(function(){
		sendCommandViaUDP("tempoDown");
	});
	$('#hihat').click(function(){
		sendCommandViaUDP("hihat");
	});
	$('#snare').click(function(){
		sendCommandViaUDP("snare");
	});
	$('#base').click(function(){
		sendCommandViaUDP("base");
	});
	$('#stop').click(function(){
		sendCommandViaUDP("terminate");
	});
	setInterval(() => {
		sendCommandViaUDP("getUpdates");
	  }, 500);
	socket.on('commandReply', function(result) {
		console.log(result);
		if (result.includes("volume")){
			$('#volumeid').val(result.split(" ")[2]);
		}
		else if (result.includes("tempo")){
			$('#tempoid').val(result.split(" ")[2]);
		}
		else if (result.includes("mode")){
			$('#modeid').val(result.split(" ")[2]);
		}
		else if (result.includes("update")){
			var updates = result.split(" ");
			var mode = "";
			if (updates[3]==1) mode = "None";
			else if (updates[3]==2) mode = "Rock";
			else mode = "Hannah Montana";
			var uptime = secondsToTime(updates[4])
			var uptimeString = uptime.h + ":" + uptime.m + ":" + uptime.s
			$('#tempoid').val(updates[1]);
			$('#volumeid').val(updates[2]);
			$('#modeid').text(mode);
			$('#status').text("Device uptime = "+uptimeString);
		}
		// var newDiv = $('<code></code>')
		// 	.text(result)
		// 	.wrapInner("<div></div>");
		// $('#messages').scrollTop($('#messages').prop('scrollHeight'));
	});
	
});

function sendCommandViaUDP(message) {
	socket.emit('daUdpCommand', message);
};

// code from stackoverflow, user R4nc1d https://stackoverflow.com/questions/37096367/how-to-convert-seconds-to-minutes-and-hours-in-javascript
function secondsToTime(secs)
{
    var hours = Math.floor(secs / (60 * 60));

    var divisor_for_minutes = secs % (60 * 60);
    var minutes = Math.floor(divisor_for_minutes / 60);

    var divisor_for_seconds = divisor_for_minutes % 60;
    var seconds = Math.ceil(divisor_for_seconds);

    var obj = {
        "h": hours,
        "m": minutes,
        "s": seconds
    };
    return obj;
}