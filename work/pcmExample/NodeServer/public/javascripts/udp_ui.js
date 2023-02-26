"use strict";
// Client-side interactions with the browser.

// Make connection to server when web page is fully loaded.
var socket = io.connect();
$(document).ready(function() {

	$('#modeNone').click(function(){
		sendCommandViaUDP("modeNone");
	});
	$('#modeRock1').click(function(){
		sendCommandViaUDP("modeRock1");
	});
	$('#modeRock2').click(function(){
		sendCommandViaUDP("modeRock2");
	});
	$('#volumeDown').click(function(){
		sendCommandViaUDP("volumeDown");
	});
	$('#volumeUp').click(function(){
		sendCommandViaUDP("volumeUp");
	});
	
	socket.on('commandReply', function(result) {
		// console.log(result);
		var newDiv = $('<code></code>')
			.text(result)
			.wrapInner("<div></div>");
		$('#volumeid').val(result);
		$('#messages').scrollTop($('#messages').prop('scrollHeight'));
	});
	
});

function sendCommandViaUDP(message) {
	socket.emit('daUdpCommand', message);
};