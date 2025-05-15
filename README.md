# Redis++
Redis server/client built with C++. This project was built for learning purposes.

Reference: [Build Your Own Redis Server](https://codingchallenges.fyi/challenges/challenge-redis).

## Features
### Server
- Concurrent client handling using non-blocking IO.
- Storing data in key-value map or in a linked list (commands: `set`, `get`, `rpush`, `lpush`, `rpop`, `lpop`).

### Client
- Can interact with other Redis servers that support RESP protocol.
- Informative feedback.

### General
- Serializing/deserializing RESP protocol (nested array parser is not implemented yet).

## Benchmark
Redis++ server performs almost as good as original Redis server.
```sh
redis-benchmark -t set,get, -n 100000 -q
```

Redis server:
```sh
SET: 141242.94 requests per second, p50=0.183 msec
GET: 137551.58 requests per second, p50=0.191 msec
```

Redis++ server:
```sh
SET: 135685.22 requests per second, p50=0.191 msec
GET: 126582.27 requests per second, p50=0.199 msec
```

