/*
 * user_webpage.h
 *
 *  Created on: 08-Jun-2021
 *      Author: harsh
 */

#ifndef INCLUDE_USER_WEBPAGE_H_
#define INCLUDE_USER_WEBPAGE_H_

#include "c_types.h"

//forward declaration
struct scanned_AP_info;

/********************************* WEB PAGE *********************************/

static char WIFI_AP_HTML [350] = "<title>ESP8266</title>\
<meta content='width=device-width,initial-scale=1'name=viewport>\
<link rel='stylesheet' href='./styles.css'>\
<script src='./script.js' ></script>\
<h2>WiFi Networks</h2>\
<form id=f method=POST>\
<input id=P type=password name=P placeholder='Enter WiFi Password'><br>\
<input id=I type=submit value=SUBMIT>\
</form>";

static char WIFI_AP_CSS [400] = "h2{\
background-color:#ff9800;\
padding:2%;\
font-style:oblique;\
border-radius:5px;\
color:#f0f8ff;\
margin:.5%\
}\
form{\
background-color:#f2f2f2;\
border-style:groove;\
border-radius:5px;\
margin:.5%\
}\
div{\
margin:10px\
}\
label{\
font-size:larger;\
font-family:sans-serif;\
margin:2px\
}\
#I,#P{\
padding:.75%;\
margin:10px;\
box-shadow:inset 0 1px 3px #ddd;\
border-radius:4px;\
border:1px solid #ccc\
}";

static char WIFI_AP_JS [800] = "document.addEventListener('DOMContentLoaded',function(){\
let e=document.getElementById('f');\
for(let t=AP.length-1;t>=0;--t){\
let n=e.childNodes[0];\
d=document.createElement('div');\
r=document.createElement('input');\
r.setAttribute('type','radio');\
r.setAttribute('name','S');\
r.setAttribute('checked','checked');\
r.setAttribute('value',AP[t]);\
r.setAttribute('id',AP[t]);\
d.appendChild(r);\
let i=document.createElement('label');\
i.setAttribute('for',AP[t]);\
i.innerHTML=AP[t];\
d.appendChild(i);\
let l=document.createElement('br');\
d.appendChild(l);\
e.insertBefore(d,n);\
}\
});";

/****************************************************************************/

//getter methods

/*******************************************************************************************
 * FunctionName	:  GetWifi_AP_HTML
 * Description	:  Used to retrieve HTML Page of Station Select Page
 * Return		:  HTML String
 ******************************************************************************************/
char* ICACHE_FLASH_ATTR GetWifi_AP_HTML(void);

/*******************************************************************************************
 * FunctionName	:  GetWifi_AP_CSS
 * Description	:  Used to retrieve CSS of HTML Page of Station Select Page
 * Return		:  CSS String
 ******************************************************************************************/
char* ICACHE_FLASH_ATTR GetWifi_AP_CSS(void);

/*******************************************************************************************
 * FunctionName	:  GetWifi_AP_CSS
 * Description	:  Used to retrieve JS of HTML Page of Station Select Page
 * Return		:  JS String
 ******************************************************************************************/
char* ICACHE_FLASH_ATTR GetWifi_AP_JS(void);

/*******************************************************************************************
 * FunctionName	:  UpdateJSData
 * Description	:  Appends scanned AP data in JS Script
 * Parameters	:  scanned_APs -- scanned APs array
 * 				   size -- size of scanned AP's array
 ******************************************************************************************/
void ICACHE_FLASH_ATTR UpdateJSData(struct scanned_AP_info *scanned_APs, uint8 size);

#endif /* INCLUDE_USER_WEBPAGE_H_ */
