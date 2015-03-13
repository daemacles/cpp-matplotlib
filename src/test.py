import json

header = b'{"msg_id":"uuid_header","msg_type":"execution_request","session":"uuid_session","username":"jay"}\n'

parent = header

metadata = b'{}\n'

content = b'{"allow_stdin":false,"code":"a = 5","silent":false,"store_history":true,"user_expressions":{},"user_variables":[]}\n' 

hmac_cpp = b'15177d1d4d974b40e02a6dade74d1ee6a02730f289f8fcb743f7cd7acf98d781'

digester = hmac.HMAC(b'43bf854e-a126-4dce-916a-9b177ca1eb2b', digestmod=hashlib.sha256)

d = digester.copy()
for s in (header, parent, metadata, content):
    d.update(s)
print d.hexdigest()
