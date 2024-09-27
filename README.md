# posch-quoted

UDP-only RFC865 Quote of the Day server, that sends some status
information in reply to UDP packets received on port 17

    C             S
    |     UDP     |
    |------------>|
    |             |
    |<------------|
    |     UDP     |
    |             |

Response format is

  "${nodename} up ${uptime} load ${loadavg} ${quoteline}"

where ${quoteline} is the first line of a quote file.

## Options

    -f quotefile        Path to quotefile (default: /etc/quote)
    -p port             Local UDP port (default: 17)

## Example


    $ ./posch-quoted -f ./quote -p 1717
    Ready.
    Received 1 byte(s) from 127.0.0.1:47870
    
    $ echo | nc -W1 -u 127.0.0.1 1717
    kex up 0-04:18:16 load 0.24 0.06 0.02 Hello World!


