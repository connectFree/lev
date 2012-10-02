# Hello World

    local web = require('web')

    web.createServer('0.0.0.0', 9000, function (client, req, res)
      res.writeHead(200, {
        ['Content-Type'] = 'text/plain', 
        ['Content-Length'] = '12'
      })
      res.fin('Hello World\n')
    end)
    print('Server running at http://127.0.0.0:9000/')

To run the server, put the code into a file called `example.lua` and execute
it with the lev program

    > lev example.lua

Server running at http://127.0.0.1:9000/

All of the examples in the documentation can be run similarly.

