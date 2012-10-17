# lev
The `lev` variable is a global variable and can be accessed from anywhere. 



## Variables

### arch
The operating system CPU architecture.

* type : string

    print('This processor architecture is ' .. lev.arch)


### buffer
Used to handle binary data. See the [buffer section][].

* type : table


### dns
The `dns` variable. See the [dns section][].

* type : table


### fs
The `fs` variable. See the [fs section][].

* type : table


### json
The `json` variable. See the [json section][].

* type : table


### mpack
The `mpack` variable. See the [mpack section][].

* type : table


### net
The `net` variable. See the [net section][].

* type : table


### pid
The PID of the process.

* type : number

    print('This process is pid ' .. lev.pid)


### pipe
The `pipe` variable. See the [pipe section][].

* type : table


### platform
The operating system platform.

* type : string

    print('This platform is ' .. lev.platform)


### signal
The `signal` variable. See the [signal section][].

* type : table


### tcp
The `tcp` variable. See the [tcp section][].

* type : table


### timer
The `timer` variable. See the [timer section][].

* type : table


### title
Set what is displayed in 'ps'.

* type : string


### udp
The `udp` variable. See the [udp section][].

* type : table


### version
A compiled-in varian that exposes version of lev.

* type : string

    print('Version: ' .. lev.version)


### versions
A variable exposing version strings of lev and its dependencies.

* type : table

Will output.

    { libuv = "0", luajit = "2.0.0-beta10", lev = "0.0.1", http_perser = "v1.0-79-g8bec3ea" }



## Functions

### abort()
This causes lev to emit an abort. This will cause lev to exit and generate a core file.

#### parameters
There is nothing.

#### returns
On success, `nil` returned.


### activate\_signal\_handler


### chdir(dir)
Changes the current working directory of the lev process.

#### parameters
* dir : The change working directory. specify string type value. (required)

#### returns
On success, `nil` returned.

    print('Starting directory: ' .. lev.cwd())
    lev.chdir('/tmp')
    print('New directory: ' .. lev.cwd())


### cpu\_info()
Get an array of tables containing information about each CPU/core installed: model, speed (in MHz), and times (an table containing the number of CPU ticks spent in: user, nice, sys, idle, and irq).

#### parameters
There is nothing.

#### returns
On success, an array of tables containing information about each CUP/core.

    {
      { times = { irq = 0, user = 418260, idle = 8541840, sys = 521500, nice = 0 }, model = "MacBookPro10,1", speed = 2600 },
      { times = { irq = 0, user = 29220, idle = 9432600, sys = 17770, nice = 0 }, model = "MacBookPro10,1", speed = 2600 },
      { times = { irq = 0, user = 371170, idle = 8871880, sys = 236590, nice = 0 }, model = "MacBookPro10,1", speed = 2600 },
      { times = { irq = 0, user = 27460, idle = 9435920, sys = 16200, nice = 0 }, model = "MacBookPro10,1", speed = 2600 },
      { times = { irq = 0, user = 344940, idle = 8926570, sys = 208110, nice = 0 }, model = "MacBookPro10,1", speed = 2600 },
      { times = { irq = 0, user = 26160, idle = 9438720, sys = 14700, nice = 0 }, model = "MacBookPro10,1", speed = 2600 },
      { times = { irq = 0, user = 329030, idle = 8963590, sys = 186980, nice = 0 }, model = "MacBookPro10,1", speed = 2600 },
      { times = { irq = 0, user = 25620, idle = 9440010, sys = 13940, nice = 0 }, model = "MacBookPro10,1", speed = 2600 }
    }

### cwd()
Get the current working directory of the lev process.

#### parameters
There is nothing.

#### returns
On success, current working directory string returned.

    print('Current directory: ' .. lev.cwd())


### environ()
Iterate the enviroment variables.

#### parameters
There is nothing.

#### returns
Return the enviroment Variables iterator.

    for k, v in lev.environ() do
      if k == 'FOO' then
        found = true
      end
    end


### execpath()
Get the absolute pathname of the executable that started the lev process.

#### parameters
There is nothing.

#### returns
On success, absolute pathname of the executable that started the lev process returned.


### exit([code])
Ends the lev process with the specified code.

#### parameters
* code : The exit code. specify number type value. (optional, default: 0)

#### returns
On success, `nil` returned.

To exit with a 'failure' code

    lev.exit(1)

The shell that executed lev should see the exit code as 1.


### get\_free\_memory()
Get the amount of free system memory in bytes.

#### parameters
There is nothing.

#### returns
On success, the amount of free system memory returned.


### get\_total\_memory()
Get the total amount of system memory in bytes.

#### parameters
There is nothing.

#### returns
On success, the total amount of system memory.


### getenv(name)
Get the enviroment variable value.

#### parameters
* name : The enviroment variable name. specify string type value. (required)

#### returns
On success, the enviroment variable value returned. if enviroment variable name is nothing, the `nil` returned.

    print('FOO: ' .. lev.getenv('FOO'))


### getgid()
Gets the group identity of the lev process. (See getgid(2).) This is the numerical group id, not the group name.

#### parameters
There is nothing.

#### returns
On success, the group identity of the lev process.

    print('Current gid: ' .. lev.getgid())


### getuid()
Gets the user identity of the lev process. (See getuid(2).) This is the numerical userid, not the username.

#### parameters
There is nothing.

#### returns
On success, the user identity of the lev process.

    print('Current uid: ' .. lev.getuid())


### handle\_type


### hrtime()
Get the current high-resolution real time seconds.

#### parameters
There is nothing.

#### returns
On success, the current high-resolution real time seconds.

    print(lev.hrtime() .. ' seconds')
    -- 13308547.876945 seconds


### interface\_addresses()
Get a list of network interfaces.

#### parameters
There is nothing.

#### returns
On success, a list of network interfaces.

    {
      en0 = {
        { address = "fe80::baf6:b1ff:fe19:b1bd", internal = false, family = "IPv6" },
        { address = "192.168.11.12", internal = false, family = "IPv4" },
        { address = "2001:c90:8407:1962:baf6:b1ff:fe19:b1bd", internal = false, family = "IPv6" },
        { address = "2001:c90:8407:1962:6c39:f9f7:103d:c91e", internal = false, family = "IPv6" }
      },
      lo0 = {
        { address = "fe80::1", internal = true, family = "IPv6" },
        { address = "127.0.0.1", internal = true, family = "IPv4" },
        { address = "::1", internal = true, family = "IPv6" }
      }
    }


### kill(pid, [signal])
Send a signal to a process.

#### parameters
* pid : The process id. specify nemuber type value. (required)
* signal : The signal value. specify number type value. (optional, default: 15(signal.SIGTERM))

#### returns
On success, `nil` returned.

    local signal = lev.signal
    local pid = 11111 -- something process id.
    lev.kill(pid, signal.SIGKILL)

Note that just because the name of this function is `lev.kill`, it is really just a signal sender, like the kill system call. The signal sent may do something other than kill the target process.


### loadavg()
Get an array containing the 1, 5, and 15 minute load averages.

#### parameters
There is nothing.

#### returns
On success, an array containing the 1, 5, and 15 minute load averages.

    { 0.251953125, 0.3984375, 0.44677734375 }


### memory\_usage()
Get an table describing the memory usage of the lev process measured in bytes.

#### parameters
There is nothing.

#### returns
On success, an table describing the memory usage of the lev process measured in bytes.

    { rss = 2228224 }


### now


### setenv(name, value)
Set the enviroment variable value.

#### parameters
* name : The enviroment variable name. specify string type value. (required)
* value : The enviroment variable value. specify string type vlaue. (required)

#### returns
On success, the `nil` returned.

    lev.setenv('FOO', 'bar')


### setgid
Sets the group identity of the lev process. (See setgid(2).)

#### parameters
* id : The group identity. specify number or string value type. (required)

#### returns
On success, the group identity of the lev process.

    print('Current gid: ' .. lev.getgid())
    lev.setuid(501)
    print('New gid: ' .. lev.getgid())


### setuid(id)
Sets the user identity of the lev process. (See setuid(2).)

#### parameters
* id : The user identity. specify number or string value type. (required)

#### returns
On success, the user identity of the lev process.

    print('Current uid: ' .. lev.getuid())
    lev.setuid(501)
    print('New uid: ' .. lev.getuid())


### timeELog


### timeHTTP


### timeHTTPLog


### umask([mask])
Sets or reads the lev process's file mode creation mask.

#### parameters
* mask : The mask. specify number or string type value. (optional)

#### returns
Returns the old mask if `mask` argument is given, otherwise returns the current mask.

    local newmask = 0644;
    local oldmask = lev.umask(newmask)
    print('Changed umask from: ' .. tonumber(oldmask, 8) .. ' to ' .. tonumber(newmask, 8))


### unsetenv(name)
remove the enviroment variable.

#### parameters
* name : The enviroment variable name. specify string type value. (required)

#### returns
On success, the `nil` returned.

    lev.unsetenv('FOO')


### update\_time


### uptime()
Get the number of seconds lev has been running.

#### parameters
There is nothing.

#### returns
On success, the number of seconds.


[buffer section]: buffer.html
[dns section]: dns.html
[fs section]: fs.html
[json section]: json.html
[mpack section]: mpack.html
[net section]: net.html
[pipe section]: pipe.html
[signal section]: signal.html
[tcp section]: tcp.html
[timer section]: timer.html
[udp section]: udp.html

