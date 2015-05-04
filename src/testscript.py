import bmemcached
client = bmemcached.Client(('127.0.0.1:11211', ), '', '')
print client.set('key', 'value')
print client.get('key')
