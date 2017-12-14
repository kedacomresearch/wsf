import requests
import json
from Config import config

def setUp():
    #startup 'timer' service
    url = '%s/service/echo'%config.service_center()
    data = json.dumps({
        'prefix': config.prefix('service.echo')
        })
    r = requests.post( url = url, data =data) 
    assert r.status_code == 200

    url = '%s/service/call'%config.service_center()
    data = json.dumps({
        'prefix': config.prefix('service.call')
        })#
    r = requests.post( url = url, data =data) 
    assert r.status_code == 200 

def tearDown():
    url = '%s/service/echo'%config.service_center()#
    r = requests.delete( url = url) 
    assert r.status_code == 200#

    url = '%s/service/call'%config.service_center()#
    r = requests.delete( url = url) 
    assert r.status_code == 200#

class testCallConcurrency():
     
   
    url = 'http://%(hostname)s%(prefix)s/call-concurrency'%{
         'hostname': 'localhost',
         'prefix': config.prefix('service.call')
    }

    
    echo_url = 'http://%(hostname)s%(prefix)s/echo'%{
         'hostname': 'localhost',
         'prefix': config.prefix('service.echo')
    }

    data=""
    params ={ "times":1000,
       'echo-service' : echo_url
    }

    def setUp(self):
        for i in range(1,1024):
            self.data +='a'

        pass#

    def tearDown(self):
        pass#

    def testGet(self):
        r = requests.get( url = self.url, params = self.params) 
        assert r.status_code == 200
        j = json.loads( r.text )
        #print "Get : microsecond per request : %s "%j["microsecond-per-request"]
        assert j["microsecond-per-request"] < 1000*10

    def testPost(self):
        r = requests.post( url = self.url, params = self.params,data = self.data) 
        assert r.status_code == 200
        j = json.loads( r.text )
        #print "Post : microsecond per request (data size=%d): %s "%( len(self.data),j["microsecond-per-request"])
        assert j["microsecond-per-request"] < 1000*10

    def testPut(self):
        r = requests.put( url = self.url, params = self.params,data = self.data) 
        assert r.status_code == 200
        j = json.loads( r.text )
        #print "Put : microsecond per request (data size=%d): %s "%( len(self.data),j["microsecond-per-request"])     
        assert j["microsecond-per-request"] < 1000*10

    def testDelete(self):
        r = requests.delete( url = self.url, params = self.params,data = self.data) 
        assert r.status_code == 200
        j = json.loads( r.text )
        #print "Delete : microsecond per request (data size=%d): %s "%( len(self.data),j["microsecond-per-request"])
        assert j["microsecond-per-request"] < 1000*10