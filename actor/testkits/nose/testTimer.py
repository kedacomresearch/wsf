import requests
import json

from Config import config

def setUp():
    #startup 'timer' service
    url = '%s/service/timer'%config.service_center()
    data = json.dumps({
        'prefix': config.prefix('service.timer')
        })

    r = requests.post( url = url, data =data) 
    assert r.status_code == 200


def tearDown():
    url = '%s/service/timer'%config.service_center()

    r = requests.delete( url = url) 
    assert r.status_code == 200


class testSetTimer():

    url = 'http://%(hostname)s%(prefix)s/timer/settimer'%{
         'hostname': 'localhost',
         'prefix': config.prefix('service.timer')
    }

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def testSetTimer(self):
        timers = [100,200,300,400,500,
        600,700,800,900,1000,
        1100,1200,1300,1400,1500]

        s = json.dumps(timers)
        
        
        r = requests.get( url = self.url,params={'timers': s}) 
        assert r.status_code == 200
        
        data = json.loads( r.text)
        for t in data:
            deta = abs(t['actual'] - t['expect'])
            
            assert deta < 80, " [inaccuracy : %sms %d%%]"%(deta, deta/t['expect'])
    
    def testLargVolume(self):
        timers = [100,200,300,400,500]
        groups = 2000/len(timers)

        s = json.dumps(timers)
        
        r = requests.get( url = self.url,params={'timers': s,'groups': groups}) 
        assert r.status_code == 200
        
        data = json.loads( r.text)
        for t in data:
            deta = abs(t['actual'] - t['expect'])
            
            assert deta < 100, " [inaccuracy : %sms %d%%]"%(deta, deta/t['expect'])
            


class testKillTimer():

    url = 'http://%(hostname)s%(prefix)s/timer/killtimer'%{
         'hostname': 'localhost',
         'prefix': config.prefix('service.timer')
    }

    def setUp(self):
        pass

    def tearDown(self):
        pass
    def testSetTimer(self):
        timers = [100,200,300,400,500, 600,700,800,900,1000 ]
        kill_time = 550
             
        r = requests.get( url = self.url,params={'kill_time': kill_time,
          'timers': json.dumps(timers) 
        }) 
        assert r.status_code == 200
        
        data = json.loads( r.text)
        for t in data:
            assert t < kill_time, " %d < %d" %(t,kill_time)
    
