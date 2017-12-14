
class Config():

    _hostname='127.0.0.1'
    _config={
        'hostname':_hostname,
        'service-center':'http://%s/service-center'%_hostname,
        #'service.timer.prefix': '/unittest',
        #'service.echo.prefix': '/unittest'
    }
    
    def prefix(self,name):
        if name.startswith("service."):
            return "/unittest/"+name

        return self._config[name+'.prefix']

    def service_center(self):
        return self._config['service-center']

config = Config()