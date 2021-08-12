#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import requests
import sys
import time

from obswebsocket import obsws, events, requests as obsrequests
import twitch

import logging
logging.basicConfig(level=logging.INFO)

sys.path.append('../')

client_id = os.environ.get('TWITCH_CLIENT_ID')
helix = twitch.Helix(client_id, os.environ.get('TWITCH_CLIENT_SECRET'))

# whitelist = ['kanacidas', 'hellotomio']
whitelist = ['copa_ajax']

host = "localhost"
port = 4444
password = "secret"


def on_event(message):
    print(u"Got message: {}".format(message))


def on_switch(message):
    print(u"You changed the scene to {}".format(message.getSceneName()))


ws = obsws(host, port, password)
ws.register(on_event)
ws.register(on_switch, events.SwitchScenes)
ws.connect()

try:
    print("OK")
    sources = ws.call(obsrequests.GetSourcesList())
    for s in sources.getSources():
        print(u"{}".format(s['name']))
    streamlinkSettings = ws.call(obsrequests.GetSourceSettings("StreamlinkSource"))
    print(u"{}".format(streamlinkSettings))
    # sceneItemList = ws.call(obsrequests.GetSceneItemList("Streamlink Test"))
    # print(u"{}".format(sceneItemList))
    streamlinkItemProperties = ws.call(obsrequests.GetSceneItemProperties("StreamlinkSource", "Streamlink Test"))
    print(u"{}".format(streamlinkItemProperties))
    time.sleep(1)
    print("END")

except KeyboardInterrupt:
    pass


def checkUser(user, client_id): # returns true if online, false if not
    TWITCH_STREAM_API_ENDPOINT_V5 = "https://api.twitch.tv/kraken/streams/{}"
    API_HEADERS = {
        'Client-ID' : client_id,
        'Accept' : 'application/vnd.twitchtv.v5+json',
    }
    url = TWITCH_STREAM_API_ENDPOINT_V5.format(user)

    print(url)
    try:
        req = requests.get(url, headers=API_HEADERS)
        jsondata = req.json()
        print(jsondata)
        if 'stream' in jsondata:
            if jsondata['stream'] is not None: # stream is online
                return True
            else:
                return False
    except Exception as e:
        print("Error checking user: ", e)
        return False


def getUserID(user, client_id):
    TWITCH_STREAM_API_ENDPOINT_V5 = "https://api.twitch.tv/kraken/users?login={}"
    API_HEADERS = {
        'Client-ID' : client_id,
        'Accept' : 'application/vnd.twitchtv.v5+json',
    }
    url = TWITCH_STREAM_API_ENDPOINT_V5.format(user)
    print(url)
    try:
        req = requests.get(url, headers=API_HEADERS)
        jsondata = req.json()
        print(jsondata)
        if len(jsondata['users']) > 0:
            if '_id' in jsondata['users'][0]:
                return jsondata['users'][0]['_id']
        return ""
    except Exception as e:
        print("Error getting user ID: ", e)
        return ""


for i in whitelist:
    userID = getUserID(i, client_id)
    if checkUser(userID, client_id):
        print(u'{} is live!'.format(i))
        streamlinkSettingsResponse = ws.call(obsrequests.GetSourceSettings("StreamlinkSource"))
        streamlinkSettings = streamlinkSettingsResponse.getSourcesettings()
        streamlinkSettings['url'] = 'https://www.twitch.tv/' + i
        ws.call(obsrequests.SetSourceSettings("StreamlinkSource", streamlinkSettings))


ws.disconnect()
