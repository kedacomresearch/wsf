import requests
import json
from Config import config

def setUp():
    #startup 'timer' service
    url = '%s/service/echo'%config.service_center()
    data = json.dumps({
        'prefix': config.prefix('service.echo')
        })#
    print url
    r = requests.post( url = url, data =data) 
    assert r.status_code == 200#


def tearDown():
    url = '%s/service/echo'%config.service_center()#

    r = requests.delete( url = url) 
    #assert r.status_code == 200#


class testEcho():#
     
   

    
    url = 'http://%(hostname)s%(prefix)s/echo'%{
         'hostname': 'localhost',
         'prefix': config.prefix('service.echo')
    }

    def setUp(self):
        pass#

    def tearDown(self):
        pass#

    def testGet(self):
        
        r = requests.get( url = self.url) 
        assert r.status_code == 200

    def testPost(self):
        data="testPost"
        r = requests.post( url = self.url,data=data) 
        assert r.status_code == 200
        assert r.text == data

    def testPut(self):
        data="testPut"
        r = requests.post( url = self.url,data=data) 
        assert r.status_code == 200
        assert r.text == data
        
    def testDelete(self):
        data="testDelete"
        r = requests.post( url = self.url,data=data) 
        assert r.status_code == 200
        assert r.text == data
