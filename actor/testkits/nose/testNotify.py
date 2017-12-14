# import requests
# import json
# import time

# from Config import config


# def setUp():
#     url = '%s/service/notify' % config.service_center()
#     data = json.dumps({'prefix': config.prefix('service.notify')})

#     r = requests.post(url=url, data=data)
#     assert r.status_code == 200


# def tearDown():
#     url = '%s/service/notify' % config.service_center()

#     r = requests.delete(url=url)
#     assert r.status_code == 200


# class testPubTopic():

#     url_pub1 = 'http://%(hostname)s%(prefix)s/topic/publish/test1' % {
#         'hostname': 'localhost',
#         'prefix': config.prefix('service.notify')
#     }
#     url_pub2 = 'http://%(hostname)s%(prefix)s/topic/publish/test2' % {
#         'hostname': 'localhost',
#         'prefix': config.prefix('service.notify')
#     }
#     url_pub3 = 'http://%(hostname)s%(prefix)s/topic/publish/test3' % {
#         'hostname': 'localhost',
#         'prefix': config.prefix('service.notify')
#     }
#     url_sub1 = 'http://%(hostname)s%(prefix)s/topic/subscribe/test1' % {
#         'hostname': 'localhost',
#         'prefix': config.prefix('service.notify')
#     }
#     url_sub2 = 'http://%(hostname)s%(prefix)s/topic/subscribe/test2' % {
#         'hostname': 'localhost',
#         'prefix': config.prefix('service.notify')
#     }
#     url_sub3 = 'http://%(hostname)s%(prefix)s/topic/subscribe/test3' % {
#         'hostname': 'localhost',
#         'prefix': config.prefix('service.notify')
#     }
#     topic = [
#         'ws://127.0.0.1:8080/topic/unittest1',
#         'ws://127.0.0.1:8080/topic/unittest2',
#         'ws://127.0.0.1:8081/topic/unittest3'
#     ]

#     def setUp(self):
#         pass

#     def tearDown(self):
#         pass

#     def test1EnableTopic(self):

#         r = requests.put(
#             url=self.url_pub1,
#             params={'operation': 'publish',
#                     'topic': self.topic[0]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_pub2,
#             params={'operation': 'publish',
#                     'topic': self.topic[1]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_pub3,
#             params={'operation': 'publish',
#                     'topic': self.topic[2]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_sub1,
#             params={'operation': 'subscribe',
#                     'topic': self.topic[0]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_sub2,
#             params={'operation': 'subscribe',
#                     'topic': self.topic[1]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_sub3,
#             params={'operation': 'subscribe',
#                     'topic': self.topic[2]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_sub3,
#             params={'operation': 'subscribe',
#                     'topic': self.topic[0]})
#         assert r.status_code == 200


#     def test3DisableTopic(self):
#         r = requests.put(
#             url=self.url_sub1,
#             params={'operation': 'unsubscribe',
#                     'topic': self.topic[0]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_sub2,
#             params={'operation': 'unsubscribe',
#                     'topic': self.topic[1]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_sub3,
#             params={'operation': 'unsubscribe',
#                     'topic': self.topic[2]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_sub3,
#             params={'operation': 'unsubscribe',
#                     'topic': self.topic[0]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_pub1,
#             params={'operation': 'unpublish',
#                     'topic': self.topic[0]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_pub2,
#             params={'operation': 'unpublish',
#                     'topic': self.topic[1]})
#         assert r.status_code == 200

#         r = requests.put(
#             url=self.url_pub3,
#             params={'operation': 'unpublish',
#                     'topic': self.topic[2]})
#         assert r.status_code == 200

#     def test2Topic(self):
#         data = [
#             'unittest msg: topic1', 'unittest msg: topic2',
#             'unittest msg: topic3'
#         ]
#         requests.put(url=self.url_pub1,
#                      params={
#                          'operation': 'push msg',
#                          'topic': self.topic[0],
#                          'times': 10
#                      },
#                      data=data[0])
#         # assert r.status_code == 200

#         requests.put(url=self.url_pub2,
#                      params={
#                          'operation': 'push msg',
#                          'topic': self.topic[1],
#                          'times': 10
#                      },
#                      data=data[1])
#         # assert r.status_code == 200

#         requests.put(url=self.url_pub3,
#                      params={
#                          'operation': 'push msg',
#                          'topic': self.topic[2],
#                          'times': 10
#                      },
#                      data=data[2])
#         # assert r.status_code == 200

