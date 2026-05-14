_This project has been created as part of the 42 curriculum by smarin-a, jdelorme_

# ft_irc

## Description

`ft_irc` is an IRC server written in **C++98**, built as part of the 42 school curriculum. The goal is to implement a functional IRC server that complies with the IRC protocol (RFC 1459), capable of handling multiple simultaneous client connections without forking, using only non-blocking I/O and a single `poll()` call.

The server supports:

- Client authentication via password (PASS)
- Nickname and username registration (NICK, USER)
- Public and private messaging (PRIVMSG)
- Channel management with operator privileges
- Channel operator commands: KICK, INVITE, TOPIC, MODE
- Channel modes: `+i` (invite-only), `+t` (topic restricted), `+k` (key/password), `+o` (operator), `+l` (user limit)

The reference IRC client used for testing and evaluation is **HexChat**.

---

## Instructions

### Compilation

Clone the repository and run:

```bash
make
```

This compiles all source files and generates the `ircserv` executable. The binary is compiled with `-Wall -Wextra -Werror -std=c++98`.

Additional Makefile rules:

| Rule | Effect |
|---|---|
| `make clean` | Remove object files |
| `make fclean` | Remove object files and binary |
| `make re` | Full recompilation |

### Execution

```bash
./ircserv <port> <password>
```

- `<port>`: TCP port the server will listen on (e.g. `6667`)
- `<password>`: Password that clients must supply to connect

Example:

```bash
./ircserv 6667 mypassword
```

### Testing with nc

To verify that partial data is handled correctly:

```bash
nc -C 127.0.0.1 6667
```

Type commands fragment by fragment using `Ctrl+D` to flush without a newline. The server reassembles all fragments before processing.

---

## Supported Commands

The server implements the following IRC commands:

### Connection and Registration

| Command | Format | Description |
|---------|--------|-------------|
| `PASS` | `PASS <password>` | Authenticate with the server password (must be sent first) |
| `NICK` | `NICK <nickname>` | Set or change your nickname |
| `USER` | `USER <username> <hostname> <servername> <realname>` | Set user information (sent once during registration) |
| `PING` | `PING <server>` | Test connection; server replies with PONG |
| `QUIT` | `QUIT [<message>]` | Disconnect from the server |

### Messaging

| Command | Format | Description |
|---------|--------|-------------|
| `PRIVMSG` | `PRIVMSG <target> :<message>` | Send a message to a user or channel |

### Channel Operations

| Command | Format | Description |
|---------|--------|-------------|
| `JOIN` | `JOIN <#channel> [<key>]` | Join a channel (creates it if it doesn't exist) |
| `PART` | `PART <#channel> [<message>]` | Leave a channel |
| `KICK` | `KICK <#channel> <user> [<reason>]` | Remove a user from a channel (operator only) |
| `INVITE` | `INVITE <nickname> <#channel>` | Invite a user to a channel (operator only) |
| `TOPIC` | `TOPIC <#channel> [<new topic>]` | View or set the channel topic |

### Channel Modes

| Command | Format | Description |
|---------|--------|-------------|
| `MODE` | `MODE <#channel> +i` | Set invite-only mode |
| `MODE` | `MODE <#channel> -i` | Remove invite-only mode |
| `MODE` | `MODE <#channel> +t` | Set topic restricted mode (only operators can change topic) |
| `MODE` | `MODE <#channel> -t` | Remove topic restricted mode |
| `MODE` | `MODE <#channel> +k <key>` | Set channel password/key |
| `MODE` | `MODE <#channel> -k` | Remove channel password/key |
| `MODE` | `MODE <#channel> +o <nickname>` | Grant operator privileges to a user |
| `MODE` | `MODE <#channel> -o <nickname>` | Remove operator privileges from a user |
| `MODE` | `MODE <#channel> +l <limit>` | Set user limit for the channel |
| `MODE` | `MODE <#channel> -l` | Remove user limit |

**Note:** The first user to join a channel becomes its operator automatically.

---

## Resources

### Protocol references

- [RFC 1459 — Internet Relay Chat Protocol](https://tools.ietf.org/html/rfc1459) — Core IRC protocol specification
- [RFC 2812 — IRC Client Protocol](https://tools.ietf.org/html/rfc2812) — Updated client-server message format
- [HexChat IRC client](https://hexchat.github.io/) — Reference client used for testing

### AI usage

AI assistance (Claude) was used during development for **debugging and code review** tasks: identifying edge cases in the command parser, reviewing numeric reply codes against the RFC, and spotting potential issues such as use-after-free conditions after client removal. All generated suggestions were reviewed, tested and validated by the team before being incorporated into the project.
