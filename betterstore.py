from aiohttp import web
import redis
r = redis.Redis()
 
async def post_handle(request):
    fruit = request.rel_url.query['fruit']
    color = request.rel_url.query['color']
    result = fruit + " color: {}".format(color)
    print(r.set(fruit, color))
    print(r.time()[0])
    r.set(fruit +":time",r.time()[0])
    return web.Response(text=str(result))
 
 
async def get_handle(request):
#    key = request.match_info.get('key', "None")
    print(request.rel_url.query)
    key = request.rel_url.query['fruit']
    print(key)
    color = r.get(key)
    timestamp = r.get(key+":time")
#    reply = color + ':' + timestamp
    return web.Response(text=str(color) + ":" + str(timestamp))
 
 
async def del_handle(request):
    key = request.rel_url.query['fruit']
    r.delete(key)
    r.delete(key+":time")
    return web.Response(text=str("that was DELETE"))
 
 
app = web.Application()
app.add_routes([web.post('/save/', post_handle),
                web.get('/show/', get_handle),
                web.delete('/del/', del_handle)])
 
if __name__ == '__main__':
    web.run_app(app)
