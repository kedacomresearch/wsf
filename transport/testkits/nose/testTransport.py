import requests
import json


def setUp():
    pass


def tearDown():
    pass


class testHttp():

    url = 'http://127.0.0.1:8081/transport/http/test'

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def testHttpRequest(self):

        r = requests.get(url=self.url)
        assert r.status_code == 200


class testWebsocket():

    url = 'http://127.0.0.1:8083/transport/ws/test'

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def testWs1PushMsg(self):
        r = requests.get(url=self.url, params={'test': 0, 'target': 20})
        assert r.status_code == 200

    def testWs2ClosePush(self):
        r = requests.get(url=self.url, params={'test': 5})
        assert r.status_code == 200

    def testWs3SendMsg(self):
        r = requests.get(url=self.url, params={'test': 1, 'target': 20})
        assert r.status_code == 200

    def testWs4CloseSend(self):
        r = requests.get(url=self.url, params={'test': 5})
        assert r.status_code == 200

    def testWs5(self):
        r = requests.get(url=self.url, params={'test': 2})
        assert r.status_code == 200

    def testWs6(self):
        r = requests.get(url=self.url, params={'test': 3})
        assert r.status_code == 200

    def testWs7(self):
        r = requests.get(url=self.url, params={'test': 4})
        assert r.status_code == 200
