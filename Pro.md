{{Box
  |BORDER = #9999FF
  |BACKGROUND = #99CCFF
  |WIDTH = 100%
  |ICON =
  |HEADING = Heads up!
  |CONTENT = This article is about the protocol for the latest '''stable''' release of Minecraft '''Java Edition''' ([[Protocol version numbers|1.20.4, protocol 765]]). For the Java Edition pre-releases, see [[Pre-release protocol]]. For the incomplete Bedrock Edition docs, see [[Bedrock Protocol]]. For the old Pocket Edition, see [[Pocket Edition Protocol Documentation]].
}}

This page presents a dissection of the current '''[https://minecraft.net/ Minecraft] protocol'''.

If you're having trouble, check out the [[Protocol FAQ|FAQ]] or ask for help in the IRC channel [ircs://irc.libera.chat:6697 #mcdevs on irc.libera.chat] ([https://wiki.vg/MCDevs More Information]).

'''Note:''' While you may use the contents of this page without restriction to create servers, clients, bots, etc; substantial reproductions of this page must be attributed IAW [https://creativecommons.org/licenses/by-sa/4.0/ CC BY-SA 4.0].

The changes between versions may be viewed at [[Protocol History]].

== Definitions ==

The Minecraft server accepts connections from TCP clients and communicates with them using ''packets''. A packet is a sequence of bytes sent over the TCP connection. The meaning of a packet depends both on its packet ID and the current state of the connection. The initial state of each connection is [[#Handshaking|Handshaking]], and state is switched using the packets [[#Handshake|Handshake]] and [[#Login Success|Login Success]].

=== Data types ===

{{:Data types}} <!-- Transcluded contents of Data types article in here — go to that page if you want to edit it -->

=== Other definitions ===

{| class="wikitable"
 |-
 ! Term
 ! Definition
 |-
 | Player
 | When used in the singular, Player always refers to the client connected to the server.
 |-
 | Entity
 | Entity refers to any item, player, mob, minecart or boat etc. See {{Minecraft Wiki|Entity|the Minecraft Wiki article}} for a full list.
 |-
 | EID
 | An EID — or Entity ID — is a 4-byte sequence used to identify a specific entity. An entity's EID is unique on the entire server.
 |-
 | XYZ
 | In this document, the axis names are the same as those shown in the debug screen (F3). Y points upwards, X points east, and Z points south.
 |-
 | Meter
 | The meter is Minecraft's base unit of length, equal to the length of a vertex of a solid block. The term “block” may be used to mean “meter” or “cubic meter”.
 |-
 | Registry
 | A table describing static, gameplay-related objects of some kind, such as the types of entities, block states or biomes. The entries of a registry are typically associated with textual or numeric identifiers, or both.

Minecraft has a unified registry system used to implement most of the registries, including blocks, items, entities, biomes and dimensions. These "ordinary" registries associate entries with both namespaced textual identifiers (see [[#Identifier]]), and signed (positive) 32-bit numeric identifiers. There is also a registry of registries listing all of the registries in the registry system. Some other registries, most notably the [[Chunk Format#Block state registry|block state registry]], are however implemented in a more ad-hoc fashion.

Some registries, such as biomes and dimensions, can be customized at runtime by the server (see [[Registry Data]]), while others, such as blocks, items and entities, are hardcoded. The contents of the hardcoded registries can be extracted via the built-in [[Data Generators]] system.
 |-
 | Block state
 | Each block in Minecraft has 0 or more properties, which in turn may have any number of possible values. These represent, for example, the orientations of blocks, poweredness states of redstone components, and so on. Each of the possible permutations of property values for a block is a distinct block state. The block state registry assigns a numeric identifier to every block state of every block.

A current list of properties and state ID ranges is found on [https://pokechu22.github.io/Burger/1.20.4.html burger].

Alternatively, the vanilla server now includes an option to export the current block state ID mapping, by running <code>java -cp minecraft_server.jar net.minecraft.data.Main --reports</code>.  See [[Data Generators]] for more information.
 |-
 | Notchian
 | The official implementation of vanilla Minecraft as developed and released by Mojang.
 |-
 | Sequence
 | The action number counter for local block changes, incremented by one when clicking a block with a hand, right clicking an item, or starting or finishing digging a block. Counter handles latency to avoid applying outdated block changes to the local world.  Also is used to revert ghost blocks created when placing blocks, using buckets, or breaking blocks.
 |}

== Packet format ==

Packets cannot be larger than 2<sup>21</sup> &minus; 1 or 2097151 bytes (the maximum that can be sent in a 3-byte VarInt).  For compressed packets, this applies to both the compressed length and uncompressed lengths.

=== Without compression ===

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | Length
 | VarInt
 | Length of Packet ID + Data
 |-
 | Packet ID
 | VarInt
 |
 |-
 | Data
 | Byte Array
 | Depends on the connection state and packet ID, see the sections below
 |}

=== With compression ===

Once a [[#Set Compression|Set Compression]] packet (with a non-negative threshold) is sent, [[wikipedia:Zlib|zlib]] compression is enabled for all following packets. The format of a packet changes slightly to include the size of the uncompressed packet.

{| class=wikitable
 ! Compressed?
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | No
 | Packet Length
 | VarInt
 | Length of (Data Length) + Compressed length of (Packet ID + Data)
 |-
 | No
 | Data Length
 | VarInt
 | Length of uncompressed (Packet ID + Data) or 0
 |-
 | rowspan="2"| Yes
 | Packet ID
 | VarInt
 | zlib compressed packet ID (see the sections below)
 |-
 | Data
 | Byte Array
 | zlib compressed packet data (see the sections below)
 |}

If the size of the buffer containing the packet data and ID (as a VarInt) is smaller than the threshold specified in the packet [[#Set Compression|Set Compression]]. It will be sent as uncompressed. This is done by setting the data length as 0. (Comparable to sending a non-compressed format with an extra 0 between the length, and packet data).

If it's larger than or equal to the threshold, then it follows the regular compressed protocol format.

Compression can be disabled by sending the packet [[#Set Compression|Set Compression]] with a negative Threshold, or not sending the Set Compression packet at all.

== Handshaking ==

=== Clientbound ===

There are no clientbound packets in the Handshaking state, since the protocol immediately switches to a different state after the client sends the first packet.

=== Serverbound ===

==== Handshake ====

This causes the server to switch into the target state.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| 0x00
 | rowspan="4"| Handshaking
 | rowspan="4"| Server
 | Protocol Version
 | [[#Definitions:varint|VarInt]]
 | See [[protocol version numbers]] (currently 765 in Minecraft 1.20.4).
 |-
 | Server Address
 | [[#Definitions:string|String]] (255)
 | Hostname or IP, e.g. localhost or 127.0.0.1, that was used to connect. The Notchian server does not use this information. Note that SRV records are a simple redirect, e.g. if _minecraft._tcp.example.com points to mc.example.org, users connecting to example.com will provide example.org as server address in addition to connecting to it.
 |-
 | Server Port
 | [[#Definitions:unsigned-short|Unsigned Short]]
 | Default is 25565. The Notchian server does not use this information.
 |-
 | Next State
 | [[#Definitions:varint|VarInt]] [[#Definitions:enum|Enum]]
 | 1 for [[#Status|Status]], 2 for [[#Login|Login]].
 |}

==== Legacy Server List Ping ====

{{Warning|This packet uses a nonstandard format. It is never length-prefixed, and the packet ID is an Unsigned Byte instead of a VarInt.}}

While not technically part of the current protocol, legacy clients may send this packet to initiate [[Server List Ping]], and modern servers should handle it correctly.
The format of this packet is a remnant of the pre-Netty age, before the switch to Netty in 1.7 brought the standard format that is recognized now. This packet merely exists to inform legacy clients that they can't join our modern server.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0xFE
 | Handshaking
 | Server
 | Payload
 | [[#Definitions:unsigned-byte|Unsigned Byte]]
 | always 1 (<code>0x01</code>).
 |}

See [[Server List Ping#1.6]] for the details of the protocol that follows this packet.

== Status ==
{{Main|Server List Ping}}

=== Clientbound ===

==== Status Response ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x00
 | rowspan="2"| Status
 | rowspan="2"| Client
 |-
 | JSON Response
 | [[#Definitions:string|String]] (32767)
 | See [[Server List Ping#Status Response]]; as with all strings this is prefixed by its length as a VarInt.
 |}

==== Ping Response (status) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x01
 | Status
 | Client
 | Payload
 | [[#Definitions:long|Long]]
 | Should be the same as sent by the client.
 |}

=== Serverbound ===

==== Status Request ====

The status can only be requested once immediately after the handshake, before any ping. The server won't respond otherwise.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x00
 | Status
 | Server
 | colspan="3"| ''no fields''
 |}

==== Ping Request (status) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x01
 | Status
 | Server
 | Payload
 | [[#Definitions:long|Long]]
 | May be any number. Notchian clients use a system-dependent time value which is counted in milliseconds.
 |}

== Login ==

The login process is as follows:

# C→S: [[#Handshake|Handshake]] with Next State set to 2 (login)
# C→S: [[#Login Start|Login Start]]
# S→C: [[#Encryption Request|Encryption Request]]
# Client auth
# C→S: [[#Encryption Response|Encryption Response]]
# Server auth, both enable encryption
# S→C: [[#Set Compression|Set Compression]] (optional)
# S→C: [[#Login Success|Login Success]]
# C→S: [[#Login Acknowledged|Login Acknowledged]]

Set Compression, if present, must be sent before Login Success. Note that anything sent after Set Compression must use the [[#With compression|Post Compression packet format]].

For unauthenticated ("cracked"/offline-mode) and integrated servers (either of the two conditions is enough for an unencrypted connection) there is no encryption. In that case [[#Login Start|Login Start]] is directly followed by [[#Login Success|Login Success]]. The Notchian server uses UUID v3 for offline player UUIDs, with the namespace “OfflinePlayer” and the value as the player’s username. For example, Notch’s offline UUID would be derived from the string “OfflinePlayer:Notch”. This is not a requirement however, the UUID may be anything.

See [[Protocol Encryption]] for details.

=== Clientbound ===

==== Disconnect (login) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x00
 | Login
 | Client
 | Reason
 | [[#Definitions:json_chat|Json Chat]]
 | The reason why the player was disconnected.
 |}

==== Encryption Request ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x01
 | rowspan="5"| Login
 | rowspan="5"| Client
 | Server ID
 | [[#Definitions:string|String]] (20)
 | Appears to be empty.
 |-
 | Public Key Length
 | [[#Definitions:varint|VarInt]]
 | Length of Public Key
 |-
 | Public Key
 | [[#Definitions:byte|Byte]] [[#Definitions:array-of|Array]]
 | The server's public key, in bytes.
 |-
 | Verify Token Length
 | [[#Definitions:varint|VarInt]]
 | Length of Verify Token. Always 4 for Notchian servers.
 |-
 | Verify Token
 | [[#Definitions:byte|Byte]] [[#Definitions:array-of|Array]]
 | A sequence of random bytes generated by the server.
 |}

See [[Protocol Encryption]] for details.

==== Login Success ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="7"| 0x02
 | rowspan="7"| Login
 | rowspan="7"| Client
 | colspan="2"| UUID
 | colspan="2"| [[#Definitions:uuid|UUID]]
 | colspan="2"|
 |-
 | colspan="2"| Username
 | colspan="2"| [[#Definitions:string|String]] (16)
 | colspan="2"|
 |-
 | colspan="2"| Number Of Properties
 | colspan="2"| [[#Definitions:varint|VarInt]]
 | Number of elements in the following array.
 |-
 | rowspan="4"| Property
 | Name
 | rowspan="4"| [[#Definitions:array-of|Array]]
 | [[#Definitions:string|String]] (32767)
 | colspan="2"|
 |-
 | Value
 | [[#Definitions:string|String]] (32767)
 | colspan="1"|
 |-
 | Is Signed
 | [[#Definitions:boolean|Boolean]]
 | colspan="2"|
 |-
 | Signature
 | [[#Definitions:optional|Optional]] [[#Definitions:string|String]] (32767)
 | Only if Is Signed is true.
 |}

==== Set Compression ====

Enables compression.  If compression is enabled, all following packets are encoded in the [[#With compression|compressed packet format]].  Negative values will disable compression, meaning the packet format should remain in the [[#Without compression|uncompressed packet format]].  However, this packet is entirely optional, and if not sent, compression will also not be enabled (the notchian server does not send the packet when compression is disabled).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x03
 | Login
 | Client
 | Threshold
 | [[#Definitions:varint|VarInt]]
 | Maximum size of a packet before it is compressed.
 |}

==== Login Plugin Request ====

Used to implement a custom handshaking flow together with [[#Login Plugin Response|Login Plugin Response]].

Unlike plugin messages in "play" mode, these messages follow a lock-step request/response scheme, where the client is expected to respond to a request indicating whether it understood. The notchian client always responds that it hasn't understood, and sends an empty payload.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x04
 | rowspan="3"| Login
 | rowspan="3"| Client
 | Message ID
 | [[#Definitions:varint|VarInt]]
 | Generated by the server - should be unique to the connection.
 |-
 | Channel
 | [[#Definitions:identifier|Identifier]]
 | Name of the [[plugin channel]] used to send the data.
 |-
 | Data
 | [[#Definitions:byte|Byte]] [[#Definitions:array-of|Array]] (1048576)
 | Any data, depending on the channel. The length of this array must be inferred from the packet length.
 |}

In Notchian client, the maximum data length is 1048576 bytes.

=== Serverbound ===

==== Login Start  ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x00
 | rowspan="2"| Login
 | rowspan="2"| Server
 | Name
 | [[#Definitions:string|String]] (16)
 | Player's Username.
 |-
 | Player UUID
 | [[#Definitions:uuid|UUID]]
 | The [[#Definitions:uuid|UUID]] of the player logging in. Unused by the Notchian server.
 |}

==== Encryption Response ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| 0x01
 | rowspan="4"| Login
 | rowspan="4"| Server
 | Shared Secret Length
 | [[#Definitions:varint|VarInt]]
 | Length of Shared Secret.
 |-
 | Shared Secret
 | [[#Definitions:byte|Byte]] [[#Definitions:array-of|Array]]
 | Shared Secret value, encrypted with the server's public key.
 |-
 | Verify Token Length
 | [[#Definitions:varint|VarInt]]
 | Length of Verify Token.
 |-
 | Verify Token
 | [[#Definitions:byte|Byte]] [[#Definitions:array-of|Array]]
 | Verify Token value, encrypted with the same public key as the shared secret.
 |}

See [[Protocol Encryption]] for details. See [[Mojang_API#Player_Certificates]] for an API to get the message signature.

==== Login Plugin Response ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x02
 | rowspan="3"| Login
 | rowspan="3"| Server
 | Message ID
 | [[#Definitions:varint|VarInt]]
 | Should match ID from server.
 |-
 | Successful
 | [[#Definitions:boolean|Boolean]]
 | <code>true</code> if the client understood the request, <code>false</code> otherwise. When <code>false</code>, no payload follows.
 |-
 | Data
 | [[#Definitions:optional|Optional]] [[#Definitions:byte|Byte]] [[#Definitions:array-of|Array]] (1048576)
 | Any data, depending on the channel. The length of this array must be inferred from the packet length.
 |}

In Notchian server, the maximum data length is 1048576 bytes.

==== Login Acknowledged ====

Acknowledgement to the [[Protocol#Login_Success|Login Success]] packet sent by the server.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x03
 | Login
 | Server
 | colspan="3"| ''no fields''
 |}

This packet switches the connection state to [[#Configuration|configuration]].

== Configuration ==

=== Clientbound ===

==== Clientbound Plugin Message (configuration) ====

{{Main|Plugin channels}}

Mods and plugins can use this to send their data. Minecraft itself uses several [[plugin channel]]s. These internal channels are in the <code>minecraft</code> namespace.

More information on how it works on [https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/ Dinnerbone's blog]. More documentation about internal and popular registered channels are [[plugin channel|here]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x00
 | rowspan="2"| Configuration
 | rowspan="2"| Client
 | Channel
 | [[#Definitions:identifier|Identifier]]
 | Name of the [[plugin channel]] used to send the data.
 |-
 | Data
 | [[#Definitions:byte|Byte]] [[#Definitions:array-of|Array]] (1048576)
 | Any data. The length of this array must be inferred from the packet length.
 |}

In Notchian client, the maximum data length is 1048576 bytes.

==== Disconnect (configuration) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x01
 | Configuration 
 | Client
 | Reason
 | Chat
 | The reason why the player was disconnected.
 |}

==== Finish Configuration ====

Sent by the server to notify the client that the configuration process has finished. The client answers with its own [[#Finish_Configuration_2|Finish Configuration]] whenever it is ready to continue.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x02
 | rowspan="1"| Configuration
 | rowspan="1"| Client
 | colspan="3"| ''no fields''
 |}

This packet switches the connection state to [[#Play|play]].

==== Clientbound Keep Alive (configuration) ====

The server will frequently send out a keep-alive, each containing a random ID. The client must respond with the same payload (see [[#Serverbound Keep Alive (configuration)|Serverbound Keep Alive]]). If the client does not respond to them for over 30 seconds, the server kicks the client. Vice versa, if the server does not send any keep-alives for 20 seconds, the client will disconnect and yields a "Timed out" exception.

The Notchian server uses a system-dependent time in milliseconds to generate the keep alive ID value.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x03
 | Configuration
 | Client
 | Keep Alive ID
 | [[#Definitions:long|Long]]
 |
 |}

==== Ping (configuration) ====

Packet is not used by the Notchian server. When sent to the client, client responds with a [[#Pong (configuration)|Pong]] packet with the same id.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x04
 | Configuration
 | Client
 | ID
 | [[#Definitions:int|Int]]
 |
 |}

==== Registry Data ====

Represents certain registries that are sent from the server and are applied on the client.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x05
 | rowspan="1"| Configuration
 | rowspan="1"| Client
 | Registry Codec
 | [[NBT|NBT Tag Compound]]
 | See [[Registry Data]].
 |}

==== Remove Resource Pack (configuration) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x06
 | rowspan="2"| Configuration
 | rowspan="2"| Client
 | Has UUID
 | [[#Definitions:boolean|Boolean]]
 | Whether a specific resource pack should be removed, or all of them.
 |-
 | UUID
 | [[#Definitions:optional|Optional]] [[#Definitions:uuid|UUID]]
 | The [[#Definitions:uuid|UUID]] of the resource pack to be removed. Only present if the previous field is true.
 |}

==== Add Resource Pack (configuration) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="6"| 0x07
 | rowspan="6"| Configuration
 | rowspan="6"| Client
 | UUID
 | UUID
 | The unique identifier of the resource pack.
 |-
 | URL
 | String (32767)
 | The URL to the resource pack.
 |-
 | Hash
 | String (40)
 | A 40 character hexadecimal, case-insensitive [[wikipedia:SHA-1|SHA-1]] hash of the resource pack file.<br />If it's not a 40 character hexadecimal string, the client will not use it for hash verification and likely waste bandwidth.
 |-
 | Forced
 | Boolean
 | The Notchian client will be forced to use the resource pack from the server. If they decline they will be kicked from the server.
 |-
 | Has Prompt Message
 | Boolean
 | Whether a custom message should be used on the resource pack prompt.
 |-
 | Prompt Message
 | Optional Chat
 | This is shown in the prompt making the client accept or decline the resource pack. Only present if the previous field is true.
 |}

==== Feature Flags ====

Used to enable and disable features, generally experimental ones, on the client.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x08
 | rowspan="2"| Configuration
 | rowspan="2"| Client
 | Total Features
 | VarInt
 | Number of features that appear in the array below.
 |-
 | Feature Flags
 | Identifier Array
 |
 |}

As of 1.20.4, the following feature flags are available:

* minecraft:vanilla - enables vanilla features</li>
* minecraft:bundle - enables support for the bundle</li>
* minecraft:trade_rebalance - enables support for the rebalanced villager trades</li>
* minecraft:update_1_21 - enables support for 1.21 features</li>

==== Update Tags (configuration) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| 0x09
 | rowspan="3"| Configuration
 | rowspan="3"| Client
 | colspan="2"| Length of the array
 | colspan="2"| VarInt
 |
 |-
 | rowspan="2"| Array of tags
 | Registry
 | rowspan="2"| Array
 | Identifier
 | Registry identifier (Vanilla expects tags for the registries <code>minecraft:block</code>, <code>minecraft:item</code>, <code>minecraft:fluid</code>, <code>minecraft:entity_type</code>, and <code>minecraft:game_event</code>)
 |-
 | Array of Tag
 | (See below)
 |
 |}

Tag arrays look like:

{| class="wikitable"
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | colspan="2"| Length
 | colspan="2"| VarInt
 | Number of elements in the following array
 |-
 | rowspan="3"| Tags
 | Tag name
 | rowspan="3"| Array
 | Identifier
 |
 |-
 | Count
 | VarInt
 | Number of elements in the following array
 |-
 | Entries
 | Array of VarInt
 | Numeric IDs of the given type (block, item, etc.). This list replaces the previous list of IDs for the given tag. If some preexisting tags are left unmentioned, a warning is printed.
 |}

See {{Minecraft Wiki|Tag}} on the Minecraft Wiki for more information, including a list of vanilla tags.

=== Serverbound ===

==== Client Information (configuration) ====

Sent when the player connects, or when settings are changed.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| 0x00
 | rowspan="8"| Configuration
 | rowspan="8"| Server
 | Locale
 | String (16)
 | e.g. <code>en_GB</code>.
 |-
 | View Distance
 | Byte
 | Client-side render distance, in chunks.
 |-
 | Chat Mode
 | VarInt Enum
 | 0: enabled, 1: commands only, 2: hidden.  See [[Chat#Client chat mode]] for more information.
 |-
 | Chat Colors
 | Boolean
 | “Colors” multiplayer setting. Can the chat be colored?
 |-
 | Displayed Skin Parts
 | Unsigned Byte
 | Bit mask, see below.
 |-
 | Main Hand
 | VarInt Enum
 | 0: Left, 1: Right.
 |-
 | Enable text filtering
 | Boolean
 | Enables filtering of text on signs and written book titles. Currently always false (i.e. the filtering is disabled)
 |-
 | Allow server listings
 | Boolean
 | Servers usually list online players, this option should let you not show up in that list.
 |}

''Displayed Skin Parts'' flags:

* Bit 0 (0x01): Cape enabled
* Bit 1 (0x02): Jacket enabled
* Bit 2 (0x04): Left Sleeve enabled
* Bit 3 (0x08): Right Sleeve enabled
* Bit 4 (0x10): Left Pants Leg enabled
* Bit 5 (0x20): Right Pants Leg enabled
* Bit 6 (0x40): Hat enabled

The most significant bit (bit 7, 0x80) appears to be unused.

==== Serverbound Plugin Message (configuration) ====

{{Main|Plugin channels}}

Mods and plugins can use this to send their data. Minecraft itself uses some [[plugin channel]]s. These internal channels are in the <code>minecraft</code> namespace.

More documentation on this: [https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/ https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/]

Note that the length of Data is known only from the packet length, since the packet has no length field of any kind.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x01
 | rowspan="2"| Configuration
 | rowspan="2"| Server
 | Channel
 | Identifier
 | Name of the [[plugin channel]] used to send the data.
 |-
 | Data
 | Byte Array (32767)
 | Any data, depending on the channel. <code>minecraft:</code> channels are documented [[plugin channel|here]]. The length of this array must be inferred from the packet length.
 |}

In Notchian server, the maximum data length is 32767 bytes.

==== Finish Configuration ====

Sent by the client to notify the client that the configuration process has finished. It is sent in response to the server's [[#Finish_Configuration|Finish Configuration]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x02
 | rowspan="1"| Configuration
 | rowspan="1"| Server
 | colspan="3"| ''no fields''
 |}

This packet switches the connection state to [[#Play|play]].

==== Serverbound Keep Alive (configuration) ====

The server will frequently send out a keep-alive (see [[#Clientbound Keep Alive (configuration)|Clientbound Keep Alive]]), each containing a random ID. The client must respond with the same packet.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x03
 | Configuration
 | Server
 | Keep Alive ID
 | Long
 |
 |}

==== Pong (configuration) ====

Response to the clientbound packet ([[#Ping (configuration)|Ping]]) with the same id.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x04
 | Configuration
 | Server
 | ID
 | Int
 | id is the same as the ping packet
 |}

==== Resource Pack Response (configuration) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3" | 0x05
 | rowspan="3" | Configuration
 | rowspan="3" | Server
 |-
 | UUID
 | UUID
 | The unique identifier of the resource pack received in the [[#Add_Resource_Pack_(configuration)|Add Resource Pack (configuration)]] request.
 |-
 | Result
 | VarInt Enum
 | Result ID (see below).
 |}

Result can be one of the following values:

{| class="wikitable"
 ! ID
 ! Result
 |-
 | 0
 | Successfully downloaded
 |-
 | 1
 | Declined
 |-
 | 2
 | Failed to download
 |-
 | 3
 | Accepted
 |-
 | 4
 | Downloaded
 |-
 | 5
 | Invalid URL
 |-
 | 6
 | Failed to reload
 |-
 | 7
 | Discarded
 |}

== Play ==

=== Clientbound ===

==== Bundle Delimiter ====

The delimiter for a bundle of packets. When received, the client should store every subsequent packet it receives, and wait until another delimiter is received. Once that happens, the client is guaranteed to process every packet in the bundle on the same tick, and the client should stop storing packets. For example, this is used to ensure [[#Spawn_Entity|Spawn Entity]] and [[#Set_Entity_Metadata|Set Entity Metadata]] happen on the same tick.

The Notchian client doesn't allow more than 4096 packets in the same bundle.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x00
 | Play
 | Client
 | colspan="3"| ''no fields''
 |}

==== Spawn Entity ====

Sent by the server when an entity (aside from [[#Spawn_Experience_Orb|Experince Orb]]) is created.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="13"| 0x01
 | rowspan="13"| Play
 | rowspan="13"| Client
 | Entity ID
 | VarInt
 | A unique integer ID mostly used in the protocol to identify the entity.
 |-
 | Entity UUID
 | UUID
 | A unique identifier that is mostly used in persistence and places where the uniqueness matters more.
 |-
 | Type
 | VarInt
 | ID in the <code>minecraft:entity_type</code> registry (see "type" field in [[Entity metadata#Entities]]).
 |-
 | X
 | Double
 |
 |-
 | Y
 | Double
 |
 |-
 | Z
 | Double
 |
 |-
 | Pitch
 | Angle
 | To get the real pitch, you must divide this by (256.0F / 360.0F)
 |-
 | Yaw
 | Angle
 | To get the real yaw, you must divide this by (256.0F / 360.0F)
 |-
 | Head Yaw
 | Angle
 | Only used by living entities, where the head of the entity may differ from the general body rotation.
 |-
 | Data
 | VarInt
 | Meaning dependent on the value of the Type field, see [[Object Data]] for details.
 |-
 | Velocity X
 | Short
 | rowspan="3"| Same units as [[#Set Entity Velocity|Set Entity Velocity]].
 |-
 | Velocity Y
 | Short
 |-
 | Velocity Z
 | Short
 |}

{{Warning2|The points listed below should be considered when this packet is used to spawn a player entity.}}
When in {{Minecraft Wiki|Server.properties#online-mode|online mode}}, the UUIDs must be valid and have valid skin blobs.
In offline mode, [[Wikipedia:Universally unique identifier#Versions 3 and 5 (namespace name-based)|UUID v3]] is used with the String <code>OfflinePlayer:&lt;player name&gt;</code>, encoded in UTF-8 (and case-sensitive). The Notchian server uses <code>UUID.nameUUIDFromBytes</code>, implemented by OpenJDK [https://github.com/AdoptOpenJDK/openjdk-jdk8u/blob/9a91972c76ddda5c1ce28b50ca38cbd8a30b7a72/jdk/src/share/classes/java/util/UUID.java#L153-L175 here].

For NPCs UUID v2 should be used. Note:

 <+Grum> i will never confirm this as a feature you know that :)

In an example UUID, <code>xxxxxxxx-xxxx-Yxxx-xxxx-xxxxxxxxxxxx</code>, the UUID version is specified by <code>Y</code>. So, for UUID v3, <code>Y</code> will always be <code>3</code>, and for UUID v2, <code>Y</code> will always be <code>2</code>.

==== Spawn Experience Orb ====

Spawns one or more experience orbs.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x02
 | rowspan="5"| Play
 | rowspan="5"| Client
 | Entity ID
 | VarInt
 |
 |-
 | X
 | Double
 |
 |-
 | Y
 | Double
 |
 |-
 | Z
 | Double
 |
 |-
 | Count
 | Short
 | The amount of experience this orb will reward once collected.
 |}

==== Entity Animation ====

Sent whenever an entity should change animation.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x03
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | VarInt
 | Player ID.
 |-
 | Animation
 | Unsigned Byte
 | Animation ID (see below).
 |}

Animation can be one of the following values:

{| class="wikitable"
 ! ID
 ! Animation
 |-
 | 0
 | Swing main arm
 |-
 | 2
 | Leave bed
 |-
 | 3
 | Swing offhand
 |-
 | 4
 | Critical effect
 |-
 | 5
 | Magic critical effect
 |}

==== Award Statistics ====

Sent as a response to [[#Client Command|Client Command]] (id 1). Will only send the changed values if previously requested.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="4"| 0x04
 | rowspan="4"| Play
 | rowspan="4"| Client
 | colspan="2"| Count
 | colspan="2"| VarInt
 | Number of elements in the following array.
 |-
 | rowspan="3"| Statistic
 | Category ID
 | rowspan="3"| Array
 | VarInt
 | See below.
 |-
 | Statistic ID
 | VarInt
 | See below.
 |-
 | Value
 | VarInt
 | The amount to set it to.
 |}

Categories (these are namespaced, but with <code>:</code> replaced with <code>.</code>):

{| class="wikitable"
 ! Name
 ! ID
 ! Registry
 |-
 | <code>minecraft.mined</code>
 | 0
 | Blocks
 |-
 | <code>minecraft.crafted</code>
 | 1
 | Items
 |-
 | <code>minecraft.used</code>
 | 2
 | Items
 |-
 | <code>minecraft.broken</code>
 | 3
 | Items
 |-
 | <code>minecraft.picked_up</code>
 | 4
 | Items
 |-
 | <code>minecraft.dropped</code>
 | 5
 | Items
 |-
 | <code>minecraft.killed</code>
 | 6
 | Entities
 |-
 | <code>minecraft.killed_by</code>
 | 7
 | Entities
 |-
 | <code>minecraft.custom</code>
 | 8
 | Custom
 |}

Blocks, Items, and Entities use block (not block state), item, and entity ids.

Custom has the following (unit only matters for clients):

{| class="wikitable"
 ! Name
 ! ID
 ! Unit
 |-
 | <code>minecraft.leave_game</code>
 | 0
 | None
 |-
 | <code>minecraft.play_one_minute</code>
 | 1
 | Time
 |-
 | <code>minecraft.time_since_death</code>
 | 2
 | Time
 |-
 | <code>minecraft.time_since_rest</code>
 | 3
 | Time
 |-
 | <code>minecraft.sneak_time</code>
 | 4
 | Time
 |-
 | <code>minecraft.walk_one_cm</code>
 | 5
 | Distance
 |-
 | <code>minecraft.crouch_one_cm</code>
 | 6
 | Distance
 |-
 | <code>minecraft.sprint_one_cm</code>
 | 7
 | Distance
 |-
 | <code>minecraft.walk_on_water_one_cm</code>
 | 8
 | Distance
 |-
 | <code>minecraft.fall_one_cm</code>
 | 9
 | Distance
 |-
 | <code>minecraft.climb_one_cm</code>
 | 10
 | Distance
 |-
 | <code>minecraft.fly_one_cm</code>
 | 11
 | Distance
 |-
 | <code>minecraft.walk_under_water_one_cm</code>
 | 12
 | Distance
 |-
 | <code>minecraft.minecart_one_cm</code>
 | 13
 | Distance
 |-
 | <code>minecraft.boat_one_cm</code>
 | 14
 | Distance
 |-
 | <code>minecraft.pig_one_cm</code>
 | 15
 | Distance
 |-
 | <code>minecraft.horse_one_cm</code>
 | 16
 | Distance
 |-
 | <code>minecraft.aviate_one_cm</code>
 | 17
 | Distance
 |-
 | <code>minecraft.swim_one_cm</code>
 | 18
 | Distance
 |-
 | <code>minecraft.strider_one_cm</code>
 | 19
 | Distance
 |-
 | <code>minecraft.jump</code>
 | 20
 | None
 |-
 | <code>minecraft.drop</code>
 | 21
 | None
 |-
 | <code>minecraft.damage_dealt</code>
 | 22
 | Damage
 |-
 | <code>minecraft.damage_dealt_absorbed</code>
 | 23
 | Damage
 |-
 | <code>minecraft.damage_dealt_resisted</code>
 | 24
 | Damage
 |-
 | <code>minecraft.damage_taken</code>
 | 25
 | Damage
 |-
 | <code>minecraft.damage_blocked_by_shield</code>
 | 26
 | Damage
 |-
 | <code>minecraft.damage_absorbed</code>
 | 27
 | Damage
 |-
 | <code>minecraft.damage_resisted</code>
 | 28
 | Damage
 |-
 | <code>minecraft.deaths</code>
 | 29
 | None
 |-
 | <code>minecraft.mob_kills</code>
 | 30
 | None
 |-
 | <code>minecraft.animals_bred</code>
 | 31
 | None
 |-
 | <code>minecraft.player_kills</code>
 | 32
 | None
 |-
 | <code>minecraft.fish_caught</code>
 | 33
 | None
 |-
 | <code>minecraft.talked_to_villager</code>
 | 34
 | None
 |-
 | <code>minecraft.traded_with_villager</code>
 | 35
 | None
 |-
 | <code>minecraft.eat_cake_slice</code>
 | 36
 | None
 |-
 | <code>minecraft.fill_cauldron</code>
 | 37
 | None
 |-
 | <code>minecraft.use_cauldron</code>
 | 38
 | None
 |-
 | <code>minecraft.clean_armor</code>
 | 39
 | None
 |-
 | <code>minecraft.clean_banner</code>
 | 40
 | None
 |-
 | <code>minecraft.clean_shulker_box</code>
 | 41
 | None
 |-
 | <code>minecraft.interact_with_brewingstand</code>
 | 42
 | None
 |-
 | <code>minecraft.interact_with_beacon</code>
 | 43
 | None
 |-
 | <code>minecraft.inspect_dropper</code>
 | 44
 | None
 |-
 | <code>minecraft.inspect_hopper</code>
 | 45
 | None
 |-
 | <code>minecraft.inspect_dispenser</code>
 | 46
 | None
 |-
 | <code>minecraft.play_noteblock</code>
 | 47
 | None
 |-
 | <code>minecraft.tune_noteblock</code>
 | 48
 | None
 |-
 | <code>minecraft.pot_flower</code>
 | 49
 | None
 |-
 | <code>minecraft.trigger_trapped_chest</code>
 | 50
 | None
 |-
 | <code>minecraft.open_enderchest</code>
 | 51
 | None
 |-
 | <code>minecraft.enchant_item</code>
 | 52
 | None
 |-
 | <code>minecraft.play_record</code>
 | 53
 | None
 |-
 | <code>minecraft.interact_with_furnace</code>
 | 54
 | None
 |-
 | <code>minecraft.interact_with_crafting_table</code>
 | 55
 | None
 |-
 | <code>minecraft.open_chest</code>
 | 56
 | None
 |-
 | <code>minecraft.sleep_in_bed</code>
 | 57
 | None
 |-
 | <code>minecraft.open_shulker_box</code>
 | 58
 | None
 |-
 | <code>minecraft.open_barrel</code>
 | 59
 | None
 |-
 | <code>minecraft.interact_with_blast_furnace</code>
 | 60
 | None
 |-
 | <code>minecraft.interact_with_smoker</code>
 | 61
 | None
 |-
 | <code>minecraft.interact_with_lectern</code>
 | 62
 | None
 |-
 | <code>minecraft.interact_with_campfire</code>
 | 63
 | None
 |-
 | <code>minecraft.interact_with_cartography_table</code>
 | 64
 | None
 |-
 | <code>minecraft.interact_with_loom</code>
 | 65
 | None
 |-
 | <code>minecraft.interact_with_stonecutter</code>
 | 66
 | None
 |-
 | <code>minecraft.bell_ring</code>
 | 67
 | None
 |-
 | <code>minecraft.raid_trigger</code>
 | 68
 | None
 |-
 | <code>minecraft.raid_win</code>
 | 69
 | None
 |-
 | <code>minecraft.interact_with_anvil</code>
 | 70
 | None
 |-
 | <code>minecraft.interact_with_grindstone</code>
 | 71
 | None
 |-
 | <code>minecraft.target_hit</code>
 | 72
 | None
 |-
 | <code>minecraft.interact_with_smithing_table</code>
 | 73
 | None
 |}

Units:

* None: just a normal number (formatted with 0 decimal places)
* Damage: value is 10 times the normal amount
* Distance: a distance in centimeters (hundredths of blocks)
* Time: a time span in ticks

==== Acknowledge Block Change ====

Acknowledges a user-initiated block change. After receiving this packet, the client will display the block state sent by the server instead of the one predicted by the client.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x05
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Sequence ID
 | VarInt
 | Represents the sequence to acknowledge, this is used for properly syncing block changes to the client after interactions.
 |}

==== Set Block Destroy Stage ====

0–9 are the displayable destroy stages and each other number means that there is no animation on this coordinate.

Block break animations can still be applied on air; the animation will remain visible although there is no block being broken.  However, if this is applied to a transparent block, odd graphical effects may happen, including water losing its transparency.  (An effect similar to this can be seen in normal gameplay when breaking ice blocks)

If you need to display several break animations at the same time you have to give each of them a unique Entity ID. The entity ID does not need to correspond to an actual entity on the client. It is valid to use a randomly generated number.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x06
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Entity ID
 | VarInt
 | The ID of the entity breaking the block.
 |-
 | Location
 | Position
 | Block Position.
 |-
 | Destroy Stage
 | Byte
 | 0–9 to set it, any other value to remove it.
 |}

==== Block Entity Data ====

Sets the block entity associated with the block at the given location.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x07
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Location
 | Position
 |
 |-
 | Type
 | VarInt
 | The type of the block entity
 |-
 | NBT Data
 | [[NBT|NBT Tag]]
 | Data to set.  May be a TAG_END (0), in which case the block entity at the given location is removed (though this is not required since the client will remove the block entity automatically on chunk unload or block removal).
 |}

==== Block Action ====

This packet is used for a number of actions and animations performed by blocks, usually non-persistent.  The client ignores the provided block type and instead uses the block state in their world.

See [[Block Actions]] for a list of values.

{{Warning2|This packet uses a block ID from the <code>minecraft:block</code> registry, not a block state.}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| 0x08
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Location
 | Position
 | Block coordinates.
 |-
 | Action ID (Byte 1)
 | Unsigned Byte
 | Varies depending on block — see [[Block Actions]].
 |-
 | Action Parameter (Byte 2)
 | Unsigned Byte
 | Varies depending on block — see [[Block Actions]].
 |-
 | Block Type
 | VarInt
 | The block type ID for the block. This value is unused by the Notchian client, as it will infer the type of block based on the given position.
 |}

==== Block Update ====

Fired whenever a block is changed within the render distance.

{{Warning2|Changing a block in a chunk that is not loaded is not a stable action.  The Notchian client currently uses a ''shared'' empty chunk which is modified for all block changes in unloaded chunks; while in 1.9 this chunk never renders in older versions the changed block will appear in all copies of the empty chunk.  Servers should avoid sending block changes in unloaded chunks and clients should ignore such packets.}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x09
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Location
 | Position
 | Block Coordinates.
 |-
 | Block ID
 | VarInt
 | The new block state ID for the block as given in the [[Chunk Format#Block state registry|block state registry]].
 |}

==== Boss Bar ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="14"| 0x0A
 | rowspan="14"| Play
 | rowspan="14"| Client
 | colspan="2"| UUID
 | UUID
 | Unique ID for this bar.
 |-
 | colspan="2"| Action
 | VarInt Enum
 | Determines the layout of the remaining packet.
 |-
 ! Action
 ! Field Name
 !
 !
 |-
 | rowspan="5"| 0: add
 | Title
 | Chat
 |
 |-
 | Health
 | Float
 | From 0 to 1. Values greater than 1 do not crash a Notchian client, and start [https://i.johni0702.de/nA.png rendering part of a second health bar] at around 1.5.
 |-
 | Color
 | VarInt Enum
 | Color ID (see below).
 |-
 | Division
 | VarInt Enum
 | Type of division (see below).
 |-
 | Flags
 | Unsigned Byte
 | Bit mask. 0x1: should darken sky, 0x2: is dragon bar (used to play end music), 0x04: create fog (previously was also controlled by 0x02).
 |-
 | 1: remove
 | ''no fields''
 | ''no fields''
 | Removes this boss bar.
 |-
 | 2: update health
 | Health
 | Float
 | ''as above''
 |-
 | 3: update title
 | Title
 | Chat
 |
 |-
 | rowspan="2"| 4: update style
 | Color
 | VarInt Enum
 | Color ID (see below).
 |-
 | Dividers
 | VarInt Enum
 | ''as above''
 |-
 | 5: update flags
 | Flags
 | Unsigned Byte
 | ''as above''
 |}

{| class="wikitable"
 ! ID
 ! Color
 |-
 | 0
 | Pink
 |-
 | 1
 | Blue
 |-
 | 2
 | Red
 |-
 | 3
 | Green
 |-
 | 4
 | Yellow
 |-
 | 5
 | Purple
 |-
 | 6
 | White
 |}

{| class="wikitable"
 ! ID
 ! Type of division
 |-
 | 0
 | No division
 |-
 | 1
 | 6 notches
 |-
 | 2
 | 10 notches
 |-
 | 3
 | 12 notches
 |-
 | 4
 | 20 notches
 |}

==== Change Difficulty ====

Changes the difficulty setting in the client's option menu

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x0B
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Difficulty
 | Unsigned Byte
 | 0: peaceful, 1: easy, 2: normal, 3: hard.
 |-
 | Difficulty locked?
 | Boolean
 |
 |}

==== Chunk Batch Finished ====

{{Need Info|Why does the formula uses <code>25</code> instead of the normal tick duration of <code>50</code>?}}

Marks the end of a chunk batch. The Notchian client marks the time it receives this packet and calculates the ellapsed duration since the [[#Chunk Batch Start|beggining of the chunk batch]]. The server uses this duration and the batch size received in this packet to estimate the number of milliseconds ellapsed per chunk received. This value is then used to calculate the desired number of chunks per tick through the formula <code>25 / millisPerChunk</code>, which is reported to the server through [[#Chunk Batch Received|Chunk Batch Received]].

The Notchian client uses the samples from the latest 15 batches to estimate the milliseconds per chunk number.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x0C
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Batch size
 | VarInt
 | Number of chunks.
 |}

==== Chunk Batch Start ====

Marks the start of a chunk batch. The Notchian client marks and stores the time it receives this packet.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x0D
 | Play
 | Client
 | colspan="3"| ''no fields''
 |}

==== Chunk Biomes ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="6"| 0x0E
 | rowspan="6"| Play
 | rowspan="6"| Client
 |-
 | colspan="2"| Number of chunks
 | colspan="2"| VarInt
 | Number of elements in the following array
 |-
 | rowspan="4"| Chunk biome data
 | Chunk Z
 | rowspan="4"| Array
 | Int
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | Chunk X
 | Int
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | Size
 | VarInt
 | Size of Data in bytes
 |-
 | Data
 | Byte array
 | Chunk [[Chunk Format#Data structure|data structure]], with [[Chunk Format#Chunk_Section|sections]] containing only the <code>Biomes</code> field
 |}

Note: The order of X and Z is inverted, because the client reads them as one big-endian Long, with Z being the upper 32 bits.

==== Clear Titles ====

Clear the client's current title information, with the option to also reset it.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x0F
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Reset
 | Boolean
 |
 |}

==== Command Suggestions Response ====

The server responds with a list of auto-completions of the last word sent to it. In the case of regular chat, this is a player username. Command names and parameters are also supported. The client sorts these alphabetically before listing them.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="8"| 0x10
 | rowspan="8"| Play
 | rowspan="8"| Client
 |-
 | colspan="2"| ID
 | colspan="2"| VarInt
 | Transaction ID.
 |-
 | colspan="2"| Start
 | colspan="2"| VarInt
 | Start of the text to replace.
 |-
 | colspan="2"| Length
 | colspan="2"| VarInt
 | Length of the text to replace.
 |-
 | colspan="2"| Count
 | colspan="2"| VarInt
 | Number of elements in the following array.
 |-
 | rowspan="3"| Matches
 | Match
 | rowspan="3"| Array
 | String (32767)
 | One eligible value to insert, note that each command is sent separately instead of in a single string, hence the need for Count.  Note that for instance this doesn't include a leading <code>/</code> on commands.
 |-
 | Has tooltip
 | Boolean
 | True if the following is present.
 |-
 | Tooltip
 | Optional Chat
 | Tooltip to display; only present if previous boolean is true.
 |}

==== Commands ====

Lists all of the commands on the server, and how they are parsed.

This is a directed graph, with one root node.  Each redirect or child node must refer only to nodes that have already been declared.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x11
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Count
 | VarInt
 | Number of elements in the following array.
 |-
 | Nodes
 | Array of [[Command Data|Node]]
 | An array of nodes.
 |-
 | Root index
 | VarInt
 | Index of the <code>root</code> node in the previous array.
 |}

For more information on this packet, see the [[Command Data]] article.

==== Close Container ====

This packet is sent from the server to the client when a window is forcibly closed, such as when a chest is destroyed while it's open. The notchian client disregards the provided window ID and closes any active window.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x12
 | Play
 | Client
 | Window ID
 | Unsigned Byte
 | This is the ID of the window that was closed. 0 for inventory.
 |}

==== Set Container Content ====
[[File:Inventory-slots.png|thumb|The inventory slots]]

Replaces the contents of a container window. Sent by the server upon initialization of a container window or the player's inventory, and in response to state ID mismatches (see [[#Click Container]]).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x13
 | rowspan="5"| Play
 | rowspan="5"| Client
 | Window ID
 | Unsigned Byte
 | The ID of window which items are being sent for. 0 for player inventory. The client ignores any packets targeting a Window ID other than the current one. However, an exception is made for the player inventory, which may be targeted at any time. (The Notchian server does not appear to utilize this special case.)
 |-
 | State ID
 | VarInt
 | A server-managed sequence number used to avoid desynchronization; see [[#Click Container]].
 |-
 | Count
 | VarInt
 | Number of elements in the following array.
 |-
 | Slot Data
 | Array of [[Slot Data|Slot]]
 |-
 | Carried Item
 | [[Slot Data|Slot]]
 | Item being dragged with the mouse.
 |}

See [[Inventory#Windows|inventory windows]] for further information about how slots are indexed.
Use [[#Open Screen|Open Screen]] to open the container on the client.

==== Set Container Property ====

This packet is used to inform the client that part of a GUI window should be updated.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x14
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Window ID
 | Unsigned Byte
 |
 |-
 | Property
 | Short
 | The property to be updated, see below.
 |-
 | Value
 | Short
 | The new value for the property, see below.
 |}

The meaning of the Property field depends on the type of the window. The following table shows the known combinations of window type and property, and how the value is to be interpreted.

{| class="wikitable"
 |-
 ! Window type
 ! Property
 ! Value
 |-
 | rowspan="4"| Furnace
 | 0: Fire icon (fuel left)
 | counting from fuel burn time down to 0 (in-game ticks)
 |-
 | 1: Maximum fuel burn time
 | fuel burn time or 0 (in-game ticks)
 |-
 | 2: Progress arrow
 | counting from 0 to maximum progress (in-game ticks)
 |-
 | 3: Maximum progress
 | always 200 on the notchian server
 |-
 | rowspan="10"| Enchantment Table
 | 0: Level requirement for top enchantment slot
 | rowspan="3"| The enchantment's xp level requirement
 |-
 | 1: Level requirement for middle enchantment slot
 |-
 | 2: Level requirement for bottom enchantment slot
 |-
 | 3: The enchantment seed
 | Used for drawing the enchantment names (in [[Wikipedia:Standard Galactic Alphabet|SGA]]) clientside.  The same seed ''is'' used to calculate enchantments, but some of the data isn't sent to the client to prevent easily guessing the entire list (the seed value here is the regular seed bitwise and <code>0xFFFFFFF0</code>).
 |-
 | 4: Enchantment ID shown on mouse hover over top enchantment slot
 | rowspan="3"| The enchantment id (set to -1 to hide it), see below for values
 |-
 | 5: Enchantment ID shown on mouse hover over middle enchantment slot
 |-
 | 6: Enchantment ID shown on mouse hover over bottom enchantment slot
 |-
 | 7: Enchantment level shown on mouse hover over the top slot
 | rowspan="3"| The enchantment level (1 = I, 2 = II, 6 = VI, etc.), or -1 if no enchant
 |-
 | 8: Enchantment level shown on mouse hover over the middle slot
 |-
 | 9: Enchantment level shown on mouse hover over the bottom slot
 |-
 | rowspan="3"| Beacon
 | 0: Power level
 | 0-4, controls what effect buttons are enabled
 |-
 | 1: First potion effect
 | {{Minecraft Wiki|Data values#Status effects|Potion effect ID}} for the first effect, or -1 if no effect
 |-
 | 2: Second potion effect
 | {{Minecraft Wiki|Data values#Status effects|Potion effect ID}} for the second effect, or -1 if no effect
 |-
 | Anvil
 | 0: Repair cost
 | The repair's cost in xp levels
 |-
 | rowspan="2"| Brewing Stand
 | 0: Brew time
 | 0 – 400, with 400 making the arrow empty, and 0 making the arrow full
 |-
 | 1: Fuel time
 | 0 - 20, with 0 making the arrow empty, and 20 making the arrow full
 |-
 | Stonecutter
 | 0: Selected recipe
 | The index of the selected recipe. -1 means none is selected.
 |-
 | Loom
 | 0: Selected pattern
 | The index of the selected pattern. 0 means none is selected, 0 is also the internal id of the "base" pattern.
 |-
 | Lectern
 | 0: Page number
 | The current page number, starting from 0.
 |}

For an enchanting table, the following numerical IDs are used:

{| class="wikitable"
 ! Numerical ID
 ! Enchantment ID
 ! Enchantment Name
 |-
 | 0
 | minecraft:protection
 | Protection
 |-
 | 1
 | minecraft:fire_protection
 | Fire Protection
 |-
 | 2
 | minecraft:feather_falling
 | Feather Falling
 |-
 | 3
 | minecraft:blast_protection
 | Blast Protection
 |-
 | 4
 | minecraft:projectile_protection
 | Projectile Protection
 |-
 | 5
 | minecraft:respiration
 | Respiration
 |-
 | 6
 | minecraft:aqua_affinity
 | Aqua Affinity
 |-
 | 7
 | minecraft:thorns
 | Thorns
 |-
 | 8
 | minecraft:depth_strider
 | Depth Strider
 |-
 | 9
 | minecraft:frost_walker
 | Frost Walker
 |-
 | 10
 | minecraft:binding_curse
 | Curse of Binding
 |-
 | 11
 | minecraft:soul_speed
 | Soul Speed
 |-
 | 12
 | minecraft:sharpness
 | Sharpness
 |-
 | 13
 | minecraft:smite
 | Smite
 |-
 | 14
 | minecraft:bane_of_arthropods
 | Bane of Arthropods
 |-
 | 15
 | minecraft:knockback
 | Knockback
 |-
 | 16
 | minecraft:fire_aspect
 | Fire Aspect
 |-
 | 17
 | minecraft:looting
 | Looting
 |-
 | 18
 | minecraft:sweeping
 | Sweeping Edge
 |-
 | 19
 | minecraft:efficiency
 | Efficiency
 |-
 | 20
 | minecraft:silk_touch
 | Silk Touch
 |-
 | 21
 | minecraft:unbreaking
 | Unbreaking
 |-
 | 22
 | minecraft:fortune
 | Fortune
 |-
 | 23
 | minecraft:power
 | Power
 |-
 | 24
 | minecraft:punch
 | Punch
 |-
 | 25
 | minecraft:flame
 | Flame
 |-
 | 26
 | minecraft:infinity
 | Infinity
 |-
 | 27
 | minecraft:luck_of_the_sea
 | Luck of the Sea
 |-
 | 28
 | minecraft:lure
 | Lure
 |-
 | 29
 | minecraft:loyalty
 | Loyalty
 |-
 | 30
 | minecraft:impaling
 | Impaling
 |-
 | 31
 | minecraft:riptide
 | Riptide
 |-
 | 32
 | minecraft:channeling
 | Channeling
 |-
 | 33
 | minecraft:multishot
 | Multishot
 |-
 | 34
 | minecraft:quick_charge
 | Quick Charge
 |-
 | 35
 | minecraft:piercing
 | Piercing
 |-
 | 36
 | minecraft:mending
 | Mending
 |-
 | 37
 | minecraft:vanishing_curse
 | Curse of Vanishing
 |}

==== Set Container Slot ====

Sent by the server when an item in a slot (in a window) is added/removed.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| 0x15
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Window ID
 | Byte
 | The window which is being updated. 0 for player inventory. The client ignores any packets targeting a Window ID other than the current one; see below for exceptions.
 |-
 | State ID
 | VarInt
 | A server-managed sequence number used to avoid desynchronization; see [[#Click Container]].
 |-
 | Slot
 | Short
 | The slot that should be updated.
 |-
 | Slot Data
 | [[Slot Data|Slot]]
 |
 |}

If Window ID is 0, the hotbar and offhand slots (slots 36 through 45) may be updated even when a different container window is open. (The Notchian server does not appear to utilize this special case.) Updates are also restricted to those slots when the player is looking at a creative inventory tab other than the survival inventory. (The Notchian server does ''not'' handle this restriction in any way, leading to [https://bugs.mojang.com/browse/MC-242392 MC-242392].)

If Window ID is -1, the item being dragged with the mouse is set. In this case, State ID and Slot are ignored.

If Window ID is -2, any slot in the player's inventory can be updated irrespective of the current container window. In this case, State ID is ignored, and the Notchian server uses a bogus value of 0. Used by the Notchian server to implement the [[#Pick Item]] functionality.

When a container window is open, the server never sends updates targeting Window ID 0&mdash;all of the [[Inventory|window types]] include slots for the player inventory. The client must automatically apply changes targeting the inventory portion of a container window to the main inventory; the server does not resend them for ID 0 when the window is closed. However, since the armor and offhand slots are only present on ID 0, updates to those slots occurring while a window is open must be deferred by the server until the window's closure.

==== Set Cooldown ====

Applies a cooldown period to all items with the given type.  Used by the Notchian server with enderpearls.  This packet should be sent when the cooldown starts and also when the cooldown ends (to compensate for lag), although the client will end the cooldown automatically. Can be applied to any item, note that interactions still get sent to the server with the item but the client does not play the animation nor attempt to predict results (i.e block placing).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x16
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Item ID
 | VarInt
 | Numeric ID of the item to apply a cooldown to.
 |-
 | Cooldown Ticks
 | VarInt
 | Number of ticks to apply a cooldown for, or 0 to clear the cooldown.
 |}

==== Chat Suggestions ====

Unused by the Notchian server. Likely provided for custom servers to send chat message completions to clients.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x17
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Action
 | VarInt Enum
 | 0: Add, 1: Remove, 2: Set
 |-
 | Count
 | VarInt
 | Number of elements in the following array.
 |-
 | Entries
 | Array of String (32767)
 |
 |}

==== Clientbound Plugin Message (play) ====

{{Main|Plugin channels}}

Mods and plugins can use this to send their data. Minecraft itself uses several [[plugin channel]]s. These internal channels are in the <code>minecraft</code> namespace.

More information on how it works on [https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/ Dinnerbone's blog]. More documentation about internal and popular registered channels are [[plugin channel|here]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x18
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Channel
 | Identifier
 | Name of the [[plugin channel]] used to send the data.
 |-
 | Data
 | Byte Array (1048576)
 | Any data. The length of this array must be inferred from the packet length.
 |}

In Notchian client, the maximum data length is 1048576 bytes.

==== Damage Event ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="9"| 0x19
 | rowspan="9"| Play
 | rowspan="9"| Client
 |-
 | colspan="2"| Entity ID
 | colspan="2"| VarInt
 | The ID of the entity taking damage
 |-
 | colspan="2"| Source Type ID
 | colspan="2"| VarInt
 | The type of damage in the <code>minecraft:damage_type</code> registry, defined by the [[Protocol#Registry_Data|Registry Data]] packet.
 |-
 | colspan="2"| Source Cause ID
 | colspan="2"| VarInt
 | The ID + 1 of the entity responsible for the damage, if present. If not present, the value is 0
 |-
 | colspan="2"| Source Direct ID
 | colspan="2"| VarInt
 | The ID + 1 of the entity that directly dealt the damage, if present. If not present, the value is 0. If this field is present:
* and damage was dealt indirectly, such as by the use of a projectile, this field will contain the ID of such projectile;
* and damage was dealt dirctly, such as by manually attacking, this field will contain the same value as Source Cause ID.
 |-
 | colspan="2"| Has Source Position
 | colspan="2"| Boolean
 | Indicates the presence of the three following fields.
The Notchian server sends the Source Position when the damage was dealt by the /damage command and a position was specified
 |-
 | colspan="2"| Source Position X
 | colspan="2"| Optional Double
 | Only present if Has Source Position is true
 |-
 | colspan="2"| Source Position Y
 | colspan="2"| Optional Double
 | Only present if Has Source Position is true
 |-
 | colspan="2"| Source Position Z
 | colspan="2"| Optional Double
 | Only present if Has Source Position is true
 |}

==== Delete Message ====

Removes a message from the client's chat. This only works for messages with signatures, system messages cannot be deleted with this packet.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x1A
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Message ID
 | VarInt
 | The message Id + 1, used for validating message signature. The next field is present only when value of this field is equal to 0.
 |-
 | Signature
 | Optional Byte Array (256)
 | The previous message's signature. Always 256 bytes and not length-prefixed.
 |}

==== Disconnect (play) ====

Sent by the server before it disconnects a client. The client assumes that the server has already closed the connection by the time the packet arrives.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x1B
 | Play
 | Client
 | Reason
 | [[#Definitions:chat|Chat]]
 | Displayed to the client when the connection terminates.
 |}

==== Disguised Chat Message ====

Sends the client a chat message, but without any message signing information.

The Notchian server uses this packet when the console is communicating with players through commands, such as <code>/say</code>, <code>/tell</code>, <code>/me</code>, among others.

{| class="wikitable
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x1C
 | rowspan="5"| Play
 | rowspan="5"| Client
 | Message
 | Chat
 | This is used as the <code>content</code> parameter when formatting the message on the client.
 |-
 | Chat Type
 | VarInt
 | The type of chat in the <code>minecraft:chat_type</code> registry, defined by the [[Protocol#Registry_Data|Registry Data]] packet.
 |-
 | Sender Name
 | Chat
 | The name of the one sending the message, usually the sender's display name.
This is used as the <code>sender</code> parameter when formatting the message on the client.
 |-
 | Has Target Name
 | Boolean
 | True if target name is present.
 |-
 | Target Name
 | Chat
 | The name of the one receiving the message, usually the receiver's display name. Only present if previous boolean is true.
This is used as the <code>target</code> parameter when formatting the message on the client.
 |}

==== Entity Event ====

Entity statuses generally trigger an animation for an entity.  The available statuses vary by the entity's type (and are available to subclasses of that type as well).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x1D
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | Int
 |
 |-
 | Entity Status
 | Byte Enum
 | See [[Entity statuses]] for a list of which statuses are valid for each type of entity.
 |}

==== Explosion ====

Sent when an explosion occurs (creepers, TNT, and ghast fireballs).

Each block in Records is set to air. Coordinates for each axis in record is int(X) + record.x

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2" | Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="17"| 0x1E
 | rowspan="17"| Play
 | rowspan="17"| Client
 | colspan="2" | X
 | Double
 |
 |-
 | colspan="2" | Y
 | Double
 |
 |-
 | colspan="2" | Z
 | Double
 |
 |-
 | colspan="2" | Strength
 | Float
 | If the strength is greater or equal to 2.0, or the block interaction is not 0 (keep), large explosion particles are used. Otherwise, small explosion particles are used.
 |-
 | colspan="2" | Record Count
 | VarInt
 | Number of elements in the following array.
 |-
 | colspan="2" | Records
 | Array of (Byte, Byte, Byte)
 | Each record is 3 signed bytes long; the 3 bytes are the XYZ (respectively) signed offsets of affected blocks.
 |-
 | colspan="2" | Player Motion X
 | Float
 | X velocity of the player being pushed by the explosion.
 |-
 | colspan="2" | Player Motion Y
 | Float
 | Y velocity of the player being pushed by the explosion.
 |-
 | colspan="2" | Player Motion Z
 | Float
 | Z velocity of the player being pushed by the explosion.
 |-
 | colspan="2" | Block Interaction
 | VarInt Enum
 | 0 = keep, 1 = destroy, 2 = destroy_with_decay, 3 = trigger_block.
 |-
 | colspan="2" | Small Explosion Particle ID
 | VarInt
 | The particle ID listed in [[#Particle|the particle data type]].
 |-
 | colspan="2" | Small Explosion Particle Data
 | Varies
 | The variable data listed in [[#Particle|the particle data type]].
 |-
 | colspan="2" | Large Explosion Particle ID
 | VarInt
 | The particle ID listed in [[#Particle|the particle data type]].
 |-
 | colspan="2" | Large Explosion Particle Data
 | Varies
 | The variable data listed in [[#Particle|the particle data type]].
 |-
 | rowspan="3" | Explosion Sound
 | Sound Name
 | Identifier
 | The name of the sound played.
 |-
 | Has Fixed Range
 | Optional Boolean
 | Whether is has fixed range.
 |-
 | Range
 | Optional Float
 | The fixed range of the sound. Only present if previous boolean is true.
 |}

==== Unload Chunk ====

Tells the client to unload a chunk column.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x1F
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Chunk Z
 | Int
 | Block coordinate divided by 16, rounded down.
 |-
 | Chunk X
 | Int
 | Block coordinate divided by 16, rounded down.
 |}

Note: The order is inverted, because the client reads this packet as one big-endian Long, with Z being the upper 32 bits.

It is legal to send this packet even if the given chunk is not currently loaded.

==== Game Event ====

Used for a wide variety of game events, from weather to bed use to game mode to demo messages.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x20
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Event
 | Unsigned Byte
 | See below.
 |-
 | Value
 | Float
 | Depends on Event.
 |}

''Events'':

{| class="wikitable"
 ! Event
 ! Effect
 ! Value
 |-
 | 0
 | No respawn block available
 | Note: Displays message 'block.minecraft.spawn.not_valid' (You have no home bed or charged respawn anchor, or it was obstructed) to the player.
 |-
 | 1
 | End raining
 |
 |-
 | 2
 | Begin raining
 |
 |-
 | 3
 | Change game mode
 | 0: Survival, 1: Creative, 2: Adventure, 3: Spectator.
 |-
 | 4
 | Win game
 | 0: Just respawn player.<br>1: Roll the credits and respawn player.<br>Note that 1 is only sent by notchian server when player has not yet achieved advancement "The end?", else 0 is sent.
 |-
 | 5
 | Demo event
 | 0: Show welcome to demo screen.<br>101: Tell movement controls.<br>102: Tell jump control.<br>103: Tell inventory control.<br>104: Tell that the demo is over and print a message about how to take a screenshot.
 |-
 | 6
 | Arrow hit player
 | Note: Sent when any player is struck by an arrow.
 |-
 | 7
 | Rain level change
 | Note: Seems to change both sky color and lighting.<br>Rain level ranging from 0 to 1.
 |-
 | 8
 | Thunder level change
 | Note: Seems to change both sky color and lighting (same as Rain level change, but doesn't start rain). It also requires rain to render by notchian client.<br>Thunder level ranging from 0 to 1.
 |-
 | 9
 | Play pufferfish sting sound
 |-
 | 10
 | Play elder guardian mob appearance (effect and sound)
 |
 |-
 | 11
 | Enable respawn screen
 |  0: Enable respawn screen.<br>1: Immediately respawn (sent when the <code>doImmediateRespawn</code> gamerule changes).
 |-
 | 12
 | Limited crafting
 | 0: Disable limited crafting.<br>1: Enable limited crafting (sent when the <code>doLimitedCrafting</code> gamerule changes).
 |-
 | 13
 | Start waiting for level chunks
 | Instructs the client to begin the waiting process for the level chunks.<br>Sent by the server after the level is cleared on the client and is being re-sent (either during the first, or subsequent reconfigurations).
 |}

==== Open Horse Screen ====

This packet is used exclusively for opening the horse GUI. [[#Open Screen|Open Screen]] is used for all other GUIs.  The client will not open the inventory if the Entity ID does not point to an horse-like animal.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x21
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Window ID
 | Unsigned Byte
 |
 |-
 | Slot count
 | VarInt
 |
 |-
 | Entity ID
 | Int
 |
 |}

==== Hurt Animation ====

Plays a bobbing animation for the entity receiving damage.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| 0x22
 | rowspan="3"| Play
 | rowspan="3"| Client
 |-
 | colspan="2"| Entity ID
 | colspan="2"| VarInt
 | The ID of the entity taking damage
 |-
 | colspan="2"| Yaw
 | colspan="2"| Float
 | The direction the damage is coming from in relation to the entity
 |}

==== Initialize World Border ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| 0x23
 | rowspan="8"| Play
 | rowspan="8"| Client
 | X
 | Double
 |
 |-
 | Z
 | Double
 |
 |-
 | Old Diameter
 | Double
 | Current length of a single side of the world border, in meters.
 |-
 | New Diameter
 | Double
 | Target length of a single side of the world border, in meters.
 |-
 | Speed
 | VarLong
 | Number of real-time ''milli''seconds until New Diameter is reached. It appears that Notchian server does not sync world border speed to game ticks, so it gets out of sync with server lag. If the world border is not moving, this is set to 0.
 |-
 | Portal Teleport Boundary
 | VarInt
 | Resulting coordinates from a portal teleport are limited to ±value. Usually 29999984.
 |-
 | Warning Blocks
 | VarInt
 | In meters.
 |-
 | Warning Time
 | VarInt
 | In seconds as set by <code>/worldborder warning time</code>.
 |}

The Notchian client determines how solid to display the warning by comparing to whichever is higher, the warning distance or whichever is lower, the distance from the current diameter to the target diameter or the place the border will be after warningTime seconds. In pseudocode:

<syntaxhighlight lang="java">
distance = max(min(resizeSpeed * 1000 * warningTime, abs(targetDiameter - currentDiameter)), warningDistance);
if (playerDistance < distance) {
    warning = 1.0 - playerDistance / distance;
} else {
    warning = 0.0;
}
</syntaxhighlight>

==== Clientbound Keep Alive (play) ====

The server will frequently send out a keep-alive, each containing a random ID. The client must respond with the same payload (see [[#Serverbound Keep Alive (play)|Serverbound Keep Alive]]). If the client does not respond to them for over 30 seconds, the server kicks the client. Vice versa, if the server does not send any keep-alives for 20 seconds, the client will disconnect and yields a "Timed out" exception.

The Notchian server uses a system-dependent time in milliseconds to generate the keep alive ID value.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x24
 | Play
 | Client
 | Keep Alive ID
 | Long
 |
 |}

==== Chunk Data and Update Light ====
{{Main|Chunk Format}}
{{See also|#Unload Chunk}}

This packet sends all block entities in the chunk (though sending them is not required; it is still legal to send them with [[#Block Entity Data|Block Entity Data]] later). The light data in this packet is the same format as in the [[#Update Light|Update Light]] packet.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="20"| 0x25
 | rowspan="20"| Play
 | rowspan="20"| Client
 | colspan="2"| Chunk X
 | colspan="2"| Int
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | colspan="2"| Chunk Z
 | colspan="2"| Int
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | colspan="2"| Heightmaps
 | colspan="2"| [[NBT]]
 | See [[Chunk Format#Heightmaps structure]]
 |-
 | colspan="2"| Size
 | colspan="2"| VarInt
 | Size of Data in bytes
 |-
 | colspan="2"| Data
 | colspan="2"| Byte array
 | See [[Chunk Format#Data structure]]
 |-
 | colspan="2"| Number of block entities
 | colspan="2"| VarInt
 | Number of elements in the following array
 |-
 | rowspan="4"| Block Entity
 | Packed XZ
 | rowspan="4"| Array
 | Unsigned Byte
 | The packed section coordinates are relative to the chunk they are in. Values 0-15 are valid. <pre>packed_xz = ((blockX & 15) << 4) | (blockZ & 15) // encode
x = packed_xz >> 4, z = packed_xz & 15 // decode</pre>
 |-
 | Y
 | Short
 | The height relative to the world
 |-
 | Type
 | VarInt
 | The type of block entity
 |-
 | Data
 | [[NBT]]
 | The block entity's data, without the X, Y, and Z values
 |-
 | colspan="2"| Sky Light Mask
 | colspan="2"| BitSet
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has data in the Sky Light array below.  The least significant bit is for blocks 16 blocks to 1 block below the min world height (one section below the world), while the most significant bit covers blocks 1 to 16 blocks above the max world height (one section above the world).
 |-
 | colspan="2"| Block Light Mask
 | colspan="2"| BitSet
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has data in the Block Light array below.  The order of bits is the same as in Sky Light Mask.
 |-
 | colspan="2"| Empty Sky Light Mask
 | colspan="2"| BitSet
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has all zeros for its Sky Light data.  The order of bits is the same as in Sky Light Mask.
 |-
 | colspan="2"| Empty Block Light Mask
 | colspan="2"| BitSet
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has all zeros for its Block Light data.  The order of bits is the same as in Sky Light Mask.
 |-
 | colspan="2"| Sky Light array count
 | colspan="2"| VarInt
 | Number of entries in the following array; should match the number of bits set in Sky Light Mask
 |-
 | rowspan="2"| Sky Light arrays
 | Length
 | rowspan="2"| Array
 | VarInt
 | Length of the following array in bytes (always 2048)
 |-
 | Sky Light array
 | Array of 2048 bytes
 | There is 1 array for each bit set to true in the sky light mask, starting with the lowest value.  Half a byte per light value. Indexed <code><nowiki>((y<<8) | (z<<4) | x) / 2 </nowiki></code> If there's a remainder, masked 0xF0 else 0x0F.
 |-
 | colspan="2"| Block Light array count
 | colspan="2"| VarInt
 | Number of entries in the following array; should match the number of bits set in Block Light Mask
 |-
 | rowspan="2"| Block Light arrays
 | Length
 | rowspan="2"| Array
 | VarInt
 | Length of the following array in bytes (always 2048)
 |-
 | Block Light array
 | Array of 2048 bytes
 | There is 1 array for each bit set to true in the block light mask, starting with the lowest value.  Half a byte per light value. Indexed <code><nowiki>((y<<8) | (z<<4) | x) / 2 </nowiki></code> If there's a remainder, masked 0xF0 else 0x0F.
 |}

Note that the Notchian client requires an [[#Set Center Chunk|Set Center Chunk]] packet when it crosses a chunk border, otherwise it'll only display render distance + 2 chunks around the chunk it spawned in.

Unlike the [[#Update Light|Update Light]] packet which uses the same format, setting the bit corresponding to a section to 0 in both of the block light or sky light masks does not appear to be useful, and the results in testing have been highly inconsistent.

==== World Event ====
Sent when a client is to play a sound or particle effect.

By default, the Minecraft client adjusts the volume of sound effects based on distance. The final boolean field is used to disable this, and instead the effect is played from 2 blocks away in the correct direction. Currently this is only used for effect 1023 (wither spawn), effect 1028 (enderdragon death), and effect 1038 (end portal opening); it is ignored on other effects.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| 0x26
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Event
 | Int
 | The event, see below.
 |-
 | Location
 | Position
 | The location of the event.
 |-
 | Data
 | Int
 | Extra data for certain events, see below.
 |-
 | Disable Relative Volume
 | Boolean
 | See above.
 |}

Events:

{| class="wikitable"
 ! ID
 ! Name
 ! Data
 |-
 ! colspan="3"| Sound
 |-
 | 1000
 | Dispenser dispenses
 |
 |-
 | 1001
 | Dispenser fails to dispense
 |
 |-
 | 1002
 | Dispenser shoots
 |
 |-
 | 1003
 | Ender eye launched
 |
 |-
 | 1004
 | Firework shot
 |
 |-
 | 1005
 | Iron door opened
 |
 |-
 | 1006
 | Wooden door opened
 |
 |-
 | 1007
 | Wooden trapdoor opened
 |
 |-
 | 1008
 | Fence gate opened
 |
 |-
 | 1009
 | Fire extinguished
 |
 |-
 | 1010
 | Play record
 | Special case, see below for more info.
 |-
 | 1011
 | Iron door closed
 |
 |-
 | 1012
 | Wooden door closed
 |
 |-
 | 1013
 | Wooden trapdoor closed
 |
 |-
 | 1014
 | Fence gate closed
 |
 |-
 | 1015
 | Ghast warns
 |
 |-
 | 1016
 | Ghast shoots
 |
 |-
 | 1017
 | Enderdragon shoots
 |
 |-
 | 1018
 | Blaze shoots
 |
 |-
 | 1019
 | Zombie attacks wood door
 |
 |-
 | 1020
 | Zombie attacks iron door
 |
 |-
 | 1021
 | Zombie breaks wood door
 |
 |-
 | 1022
 | Wither breaks block
 |
 |-
 | 1023
 | Wither spawned
 |
 |-
 | 1024
 | Wither shoots
 |
 |-
 | 1025
 | Bat takes off
 |
 |-
 | 1026
 | Zombie infects
 |
 |-
 | 1027
 | Zombie villager converted
 |
 |-
 | 1028
 | Ender dragon death
 |
 |-
 | 1029
 | Anvil destroyed
 |
 |-
 | 1030
 | Anvil used
 |
 |-
 | 1031
 | Anvil landed
 |
 |-
 | 1032
 | Portal travel
 |
 |-
 | 1033
 | Chorus flower grown
 |
 |-
 | 1034
 | Chorus flower died
 |
 |-
 | 1035
 | Brewing stand brewed
 |
 |-
 | 1036
 | Iron trapdoor opened
 |
 |-
 | 1037
 | Iron trapdoor closed
 |
 |-
 | 1038
 | End portal created in overworld
 |
 |-
 | 1039
 | Phantom bites
 |
 |-
 | 1040
 | Zombie converts to drowned
 |
 |-
 | 1041
 | Husk converts to zombie by drowning
 |
 |-
 | 1042
 | Grindstone used
 |
 |-
 | 1043
 | Book page turned
 |
 |-
 |-
 ! colspan="3"| Particle
 |-
 | 1500
 | Composter composts
 |
 |-
 | 1501
 | Lava converts block (either water to stone, or removes existing blocks such as torches)
 |
 |-
 | 1502
 | Redstone torch burns out
 |
 |-
 | 1503
 | Ender eye placed
 |
 |-
 | 2000
 | Spawns 10 smoke particles, e.g. from a fire
 | Direction, see below.
 |-
 | 2001
 | Block break + block break sound
 | Block state ID (see [[Chunk Format#Block state registry]]).
 |-
 | 2002
 | Splash potion. Particle effect + glass break sound.
 | RGB color as an integer (e.g. 8364543 for #7FA1FF).
 |-
 | 2003
 | Eye of Ender entity break animation — particles and sound
 |
 |-
 | 2004
 | Mob spawn particle effect: smoke + flames
 |
 |-
 | 2005
 | Bonemeal particles
 | How many particles to spawn (if set to 0, 15 are spawned).
 |-
 | 2006
 | Dragon breath
 |
 |-
 | 2007
 | Instant splash potion. Particle effect + glass break sound.
 | RGB color as an integer (e.g. 8364543 for #7FA1FF).
 |-
 | 2008
 | Ender dragon destroys block
 |
 |-
 | 2009
 | Wet sponge vaporizes in nether
 |
 |-
 | 3000
 | End gateway spawn
 |
 |-
 | 3001
 | Enderdragon growl
 |
 |-
 | 3002
 | Electric spark
 |
 |-
 | 3003
 | Copper apply wax
 |
 |-
 | 3004
 | Copper remove wax
 |
 |-
 | 3005
 | Copper scrape oxidation
 |
 |}

Smoke directions:

{| class="wikitable"
 ! ID
 ! Direction
 |-
 | 0
 | Down
 |-
 | 1
 | Up
 |-
 | 2
 | North
 |-
 | 3
 | South
 |-
 | 4
 | West
 |-
 | 5
 | East
 |}

Play record: This is actually a special case within this packet. You can start/stop a record at a specific location. Use a valid {{Minecraft Wiki|Music Discs|Record ID}} to start a record (or overwrite a currently playing one), any other value will stop the record.  See [[Data Generators]] for information on item IDs.

==== Particle ====

Displays the named particle

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="11"| 0x27
 | rowspan="11"| Play
 | rowspan="11"| Client
 | Particle ID
 | VarInt
 | The particle ID listed in [[#Particle|the particle data type]].
 |-
 | Long Distance
 | Boolean
 | If true, particle distance increases from 256 to 65536.
 |-
 | X
 | Double
 | X position of the particle.
 |-
 | Y
 | Double
 | Y position of the particle.
 |-
 | Z
 | Double
 | Z position of the particle.
 |-
 | Offset X
 | Float
 | This is added to the X position after being multiplied by <code>random.nextGaussian()</code>.
 |-
 | Offset Y
 | Float
 | This is added to the Y position after being multiplied by <code>random.nextGaussian()</code>.
 |-
 | Offset Z
 | Float
 | This is added to the Z position after being multiplied by <code>random.nextGaussian()</code>.
 |-
 | Max Speed
 | Float
 |
 |-
 | Particle Count
 | Int
 | The number of particles to create.
 |-
 | Data
 | Varies
 | The variable data listed in [[#Particle|the particle data type]].
 |}

==== Update Light ====

Updates light levels for a chunk.  See {{Minecraft Wiki|Light}} for information on how lighting works in Minecraft.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="12"| 0x28
 | rowspan="12"| Play
 | rowspan="12"| Client
 | colspan="2"| Chunk X
 | colspan="2"| VarInt
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | colspan="2"| Chunk Z
 | colspan="2"| VarInt
 | Chunk coordinate (block coordinate divided by 16, rounded down)
 |-
 | colspan="2"| Sky Light Mask
 | colspan="2"| BitSet
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has data in the Sky Light array below.  The least significant bit is for blocks 16 blocks to 1 block below the min world height (one section below the world), while the most significant bit covers blocks 1 to 16 blocks above the max world height (one section above the world).
 |-
 | colspan="2"| Block Light Mask
 | colspan="2"| BitSet
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has data in the Block Light array below.  The order of bits is the same as in Sky Light Mask.
 |-
 | colspan="2"| Empty Sky Light Mask
 | colspan="2"| BitSet
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has all zeros for its Sky Light data.  The order of bits is the same as in Sky Light Mask.
 |-
 | colspan="2"| Empty Block Light Mask
 | colspan="2"| BitSet
 | BitSet containing bits for each section in the world + 2.  Each set bit indicates that the corresponding 16×16×16 chunk section has all zeros for its Block Light data.  The order of bits is the same as in Sky Light Mask.
 |-
 | colspan="2"| Sky Light array count
 | colspan="2"| VarInt
 | Number of entries in the following array; should match the number of bits set in Sky Light Mask
 |-
 | rowspan="2"| Sky Light arrays
 | Length
 | rowspan="2"| Array
 | VarInt
 | Length of the following array in bytes (always 2048)
 |-
 | Sky Light array
 | Array of 2048 bytes
 | There is 1 array for each bit set to true in the sky light mask, starting with the lowest value.  Half a byte per light value.
 |-
 | colspan="2"| Block Light array count
 | colspan="2"| VarInt
 | Number of entries in the following array; should match the number of bits set in Block Light Mask
 |-
 | rowspan="2"| Block Light arrays
 | Length
 | rowspan="2"| Array
 | VarInt
 | Length of the following array in bytes (always 2048)
 |-
 | Block Light array
 | Array of 2048 bytes
 | There is 1 array for each bit set to true in the block light mask, starting with the lowest value.  Half a byte per light value.
 |}

A bit will never be set in both the block light mask and the empty block light mask, though it may be present in neither of them (if the block light does not need to be updated for the corresponding chunk section).  The same applies to the sky light mask and the empty sky light mask.

==== Login (play) ====

{{Need Info|Although the number of portal cooldown ticks is included in this packet, the whole portal usage process is still dictated entirely by the server. What kind of effect does this value have on the client, if any?}}

See [[Protocol Encryption]] for information on logging in.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="21"| 0x29
 | rowspan="21"| Play
 | rowspan="21"| Client
 | Entity ID
 | Int
 | The player's Entity ID (EID).
 |-
 | Is hardcore
 | Boolean
 |
 |-
 | Dimension Count
 | VarInt
 | Size of the following array.
 |-
 | Dimension Names
 | Array of Identifier
 | Identifiers for all dimensions on the server.
 |-
 | Max Players
 | VarInt
 | Was once used by the client to draw the player list, but now is ignored.
 |-
 | View Distance
 | VarInt
 | Render distance (2-32).
 |-
 | Simulation Distance
 | VarInt
 | The distance that the client will process specific things, such as entities.
 |-
 | Reduced Debug Info
 | Boolean
 | If true, a Notchian client shows reduced information on the {{Minecraft Wiki|debug screen}}.  For servers in development, this should almost always be false.
 |-
 | Enable respawn screen
 | Boolean
 | Set to false when the doImmediateRespawn gamerule is true.
 |-
 | Do limited crafting
 | Boolean
 | Whether players can only craft recipes they have already unlocked. Currently unused by the client.
 |-
 | Dimension Type
 | Identifier
 | The type of dimension in the <code>minecraft:dimension_type</code> registry, defined by the [[Protocol#Registry_Data|Registry Data]] packet.
 |-
 | Dimension Name
 | Identifier
 | Name of the dimension being spawned into.
 |-
 | Hashed seed
 | Long
 | First 8 bytes of the SHA-256 hash of the world's seed. Used client side for biome noise 
 |-
 | Game mode
 | Unsigned Byte
 | 0: Survival, 1: Creative, 2: Adventure, 3: Spectator.
 |-
 | Previous Game mode
 | Byte
 | -1: Undefined (null), 0: Survival, 1: Creative, 2: Adventure, 3: Spectator. The previous game mode. Vanilla client uses this for the debug (F3 + N & F3 + F4) game mode switch. (More information needed)
 |-
 | Is Debug
 | Boolean
 | True if the world is a {{Minecraft Wiki|debug mode}} world; debug mode worlds cannot be modified and have predefined blocks.
 |-
 | Is Flat
 | Boolean
 | True if the world is a {{Minecraft Wiki|superflat}} world; flat worlds have different void fog and a horizon at y=0 instead of y=63.
 |-
 | Has death location
 | Boolean
 | If true, then the next two fields are present.
 |-
 | Death dimension name
 | Optional Identifier
 | Name of the dimension the player died in.
 |-
 | Death location
 | Optional Position
 | The location that the player died at.
 |-
 | Portal cooldown
 | VarInt
 | The number of ticks until the player can use the portal again.
 |}

==== Map Data ====

Updates a rectangular area on a {{Minecraft Wiki|map}} item.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="17"| 0x2A
 | rowspan="17"| Play
 | rowspan="17"| Client
 | colspan="2"| Map ID
 | colspan="2"| VarInt
 | Map ID of the map being modified
 |-
 | colspan="2"| Scale
 | colspan="2"| Byte
 | From 0 for a fully zoomed-in map (1 block per pixel) to 4 for a fully zoomed-out map (16 blocks per pixel)
 |-
 | colspan="2"| Locked
 | colspan="2"| Boolean
 | True if the map has been locked in a cartography table
 |-
 | colspan="2"| Has Icons
 | colspan="2"| Boolean
 |
 |-
 | colspan="2"| Icon Count
 | colspan="2"| Optional VarInt
 | Number of elements in the following array. Only present if previous Boolean is true.
 |-
 | rowspan="6"| Icon
 | Type
 | rowspan="6"| Optional Array
 | VarInt Enum
 | See below
 |-
 | X
 | Byte
 | Map coordinates: -128 for furthest left, +127 for furthest right
 |-
 | Z
 | Byte
 | Map coordinates: -128 for highest, +127 for lowest
 |-
 | Direction
 | Byte
 | 0-15
 |-
 | Has Display Name
 | Boolean
 |
 |-
 | Display Name
 | Optional Chat
 | Only present if previous Boolean is true
 |-
 | colspan="2"| Columns
 | colspan="2"| Unsigned Byte
 | Number of columns updated
 |-
 | colspan="2"| Rows
 | colspan="2"| Optional Unsigned Byte
 | Only if Columns is more than 0; number of rows updated
 |-
 | colspan="2"| X
 | colspan="2"| Optional Byte
 | Only if Columns is more than 0; x offset of the westernmost column
 |-
 | colspan="2"| Z
 | colspan="2"| Optional Byte
 | Only if Columns is more than 0; z offset of the northernmost row
 |-
 | colspan="2"| Length
 | colspan="2"| Optional VarInt
 | Only if Columns is more than 0; length of the following array
 |-
 | colspan="2"| Data
 | colspan="2"| Optional Array of Unsigned Byte
 | Only if Columns is more than 0; see {{Minecraft Wiki|Map item format}}
 |}

For icons, a direction of 0 is a vertical icon and increments by 22.5&deg; (360/16).

Types are based off of rows and columns in <code>map_icons.png</code>:

{| class="wikitable"
 |-
 ! Icon type
 ! Result
 |-
 | 0
 | White arrow (players)
 |-
 | 1
 | Green arrow (item frames)
 |-
 | 2
 | Red arrow
 |-
 | 3
 | Blue arrow
 |-
 | 4
 | White cross
 |-
 | 5
 | Red pointer
 |-
 | 6
 | White circle (off-map players)
 |-
 | 7
 | Small white circle (far-off-map players)
 |-
 | 8
 | Mansion
 |-
 | 9
 | Temple
 |-
 | 10
 | White Banner
 |-
 | 11
 | Orange Banner
 |-
 | 12
 | Magenta Banner
 |-
 | 13
 | Light Blue Banner
 |-
 | 14
 | Yellow Banner
 |-
 | 15
 | Lime Banner
 |-
 | 16
 | Pink Banner
 |-
 | 17
 | Gray Banner
 |-
 | 18
 | Light Gray Banner
 |-
 | 19
 | Cyan Banner
 |-
 | 20
 | Purple Banner
 |-
 | 21
 | Blue Banner
 |-
 | 22
 | Brown Banner
 |-
 | 23
 | Green Banner
 |-
 | 24
 | Red Banner
 |-
 | 25
 | Black Banner
 |-
 | 26
 | Treasure marker
 |}

==== Merchant Offers ====

The list of trades a villager NPC is offering.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="16"| 0x2B
 | rowspan="16"| Play
 | rowspan="16"| Client
 | colspan="2"| Window ID
 | colspan="2"| VarInt
 | The ID of the window that is open; this is an int rather than a byte.
 |-
 | colspan="2"| Size
 | colspan="2"| VarInt
 | The number of trades in the following array.
 |-
 | rowspan="10"| Trades
 | Input item 1
 | rowspan="10"| Array
 | [[Slot]]
 | The first item the player has to supply for this villager trade. The count of the item stack is the default "price" of this trade.
 |-
 | Output item
 | [[Slot]]
 | The item the player will receive from this villager trade.
 |-
 | Input item 2
 | [[Slot]]
 | The second item the player has to supply for this villager trade. May be an empty slot.
 |-
 | Trade disabled
 | Boolean
 | True if the trade is disabled; false if the trade is enabled.
 |-
 | Number of trade uses
 | Int
 | Number of times the trade has been used so far. If equal to the maximum number of trades, the client will display a red X.
 |-
 | Maximum number of trade uses
 | Int
 | Number of times this trade can be used before it's exhausted.
 |-
 | XP
 | Int
 | Amount of XP the villager will earn each time the trade is used.
 |-
 | Special Price
 | Int
 | Can be zero or negative. The number is added to the price when an item is discounted due to player reputation or other effects.
 |-
 | Price Multiplier
 | Float
 | Can be low (0.05) or high (0.2). Determines how much demand, player reputation, and temporary effects will adjust the price.
 |-
 | Demand
 | Int
 | If positive, causes the price to increase. Negative values seem to be treated the same as zero.
 |-
 | colspan="2"| Villager level
 | colspan="2"| VarInt
 | Appears on the trade GUI; meaning comes from the translation key <code>merchant.level.</code> + level.
1: Novice, 2: Apprentice, 3: Journeyman, 4: Expert, 5: Master.
 |-
 | colspan="2"| Experience
 | colspan="2"| VarInt
 | Total experience for this villager (always 0 for the wandering trader).
 |-
 | colspan="2"| Is regular villager
 | colspan="2"| Boolean
 | True if this is a regular villager; false for the wandering trader.  When false, hides the villager level and some other GUI elements.
 |-
 | colspan="2"| Can restock
 | colspan="2"| Boolean
 | True for regular villagers and false for the wandering trader. If true, the "Villagers restock up to two times per day." message is displayed when hovering over disabled trades.
 |}

Modifiers can increase or decrease the number of items for the first input slot. The second input slot and the output slot never change the nubmer of items. The number of items may never be less than 1, and never more than the stack size. If special price and demand are both zero, only the default price is displayed. If either is non-zero, then the adjusted price is displayed next to the crossed-out default price. The adjusted prices is calculated as follows:

Adjusted price = default price + floor(default price x multiplier x demand) + special price

[[File:1.14-merchant-slots.png|thumb|The merchant UI, for reference]]
{{-}}

==== Update Entity Position ====

This packet is sent by the server when an entity moves less then 8 blocks; if an entity moves more than 8 blocks [[#Teleport Entity|Teleport Entity]] should be sent instead.

This packet allows at most 8 blocks movement in any direction, because short range is from -32768 to 32767. And <code>32768 / (128 * 32)</code> = 8.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x2C
 | rowspan="5"| Play
 | rowspan="5"| Client
 | Entity ID
 | VarInt
 |
 |-
 | Delta X
 | Short
 | Change in X position as <code>(currentX * 32 - prevX * 32) * 128</code>.
 |-
 | Delta Y
 | Short
 | Change in Y position as <code>(currentY * 32 - prevY * 32) * 128</code>.
 |-
 | Delta Z
 | Short
 | Change in Z position as <code>(currentZ * 32 - prevZ * 32) * 128</code>.
 |-
 | On Ground
 | Boolean
 |
 |}

==== Update Entity Position and Rotation ====

This packet is sent by the server when an entity rotates and moves. Since a short range is limited from -32768 to 32767, and movement is offset of fixed-point numbers, this packet allows at most 8 blocks movement in any direction. (<code>-32768 / (32 * 128) == -8</code>)

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="7"| 0x2D
 | rowspan="7"| Play
 | rowspan="7"| Client
 | Entity ID
 | VarInt
 |
 |-
 | Delta X
 | Short
 | Change in X position as <code>(currentX * 32 - prevX * 32) * 128</code>.
 |-
 | Delta Y
 | Short
 | Change in Y position as <code>(currentY * 32 - prevY * 32) * 128</code>.
 |-
 | Delta Z
 | Short
 | Change in Z position as <code>(currentZ * 32 - prevZ * 32) * 128</code>.
 |-
 | Yaw
 | Angle
 | New angle, not a delta.
 |-
 | Pitch
 | Angle
 | New angle, not a delta.
 |-
 | On Ground
 | Boolean
 |
 |}

==== Update Entity Rotation ====

This packet is sent by the server when an entity rotates.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| 0x2E
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Entity ID
 | VarInt
 |
 |-
 | Yaw
 | Angle
 | New angle, not a delta.
 |-
 | Pitch
 | Angle
 | New angle, not a delta.
 |-
 | On Ground
 | Boolean
 |
 |}

==== Move Vehicle ====

Note that all fields use absolute positioning and do not allow for relative positioning.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x2F
 | rowspan="5"| Play
 | rowspan="5"| Client
 | X
 | Double
 | Absolute position (X coordinate).
 |-
 | Y
 | Double
 | Absolute position (Y coordinate).
 |-
 | Z
 | Double
 | Absolute position (Z coordinate).
 |-
 | Yaw
 | Float
 | Absolute rotation on the vertical axis, in degrees.
 |-
 | Pitch
 | Float
 | Absolute rotation on the horizontal axis, in degrees.
 |}

==== Open Book ====

Sent when a player right clicks with a signed book. This tells the client to open the book GUI.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x30
 | Play
 | Client
 | Hand
 | VarInt Enum
 | 0: Main hand, 1: Off hand .
 |}

==== Open Screen ====

This is sent to the client when it should open an inventory, such as a chest, workbench, furnace, or other container. Resending this packet with already existing window id, will update the window title and window type without closing the window.

This message is not sent to clients opening their own inventory, nor do clients inform the server in any way when doing so. From the server's perspective, the inventory is always "open" whenever no other windows are.

For horses, use [[#Open Horse Screen|Open Horse Screen]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x31
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Window ID
 | VarInt
 | An identifier for the window to be displayed. Notchian server implementation is a counter, starting at 1. There can only be one window at a time; this is only used to ignore outdated packets targeting already-closed windows. Note also that the Window ID field in most other packets is only a single byte, and indeed, the Notchian server wraps around after 100.
 |-
 | Window Type
 | VarInt
 | The window type to use for display. Contained in the <code>minecraft:menu</code> registry; see [[Inventory]] for the different values.
 |-
 | Window Title
 | Chat
 | The title of the window.
 |}

==== Open Sign Editor ====

Sent when the client has placed a sign and is allowed to send [[#Update Sign|Update Sign]].  There must already be a sign at the given location (which the client does not do automatically) - send a [[#Block Update|Block Update]] first.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2" | 0x32
 | rowspan="2" | Play
 | rowspan="2" | Client
 | Location
 | Position
 |
 |-
 | Is Front Text
 | Boolean
 | Whether the opened editor is for the front or on the back of the sign
 |}

==== Ping (play) ====

Packet is not used by the Notchian server. When sent to the client, client responds with a [[#Pong (play)|Pong]] packet with the same id.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x33
 | Play
 | Client
 | ID
 | Int
 |
 |}

==== Ping Response (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x34
 | Play
 | Client
 | Payload
 | Long
 | Should be the same as sent by the client.
 |}

==== Place Ghost Recipe ====

Response to the serverbound packet ([[#Place Recipe|Place Recipe]]), with the same recipe ID. Appears to be used to notify the UI.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x35
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Window ID
 | Byte
 |
 |-
 | Recipe
 | Identifier
 | A recipe ID.
 |}

==== Player Abilities ====

The latter 2 floats are used to indicate the flying speed and field of view respectively, while the first byte is used to determine the value of 4 booleans.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x36
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Flags
 | Byte
 | Bit field, see below.
 |-
 | Flying Speed
 | Float
 | 0.05 by default.
 |-
 | Field of View Modifier
 | Float
 | Modifies the field of view, like a speed potion. A Notchian server will use the same value as the movement speed sent in the [[#Update Attributes|Update Attributes]] packet, which defaults to 0.1 for players.
 |}

About the flags:

{| class="wikitable"
 |-
 ! Field
 ! Bit
 |-
 | Invulnerable
 | 0x01
 |-
 | Flying
 | 0x02
 |-
 | Allow Flying
 | 0x04
 |-
 | Creative Mode (Instant Break)
 | 0x08
 |}

==== Player Chat Message ====

Sends the client a chat message from a player. 

Currently a lot is unknown about this packet, blank descriptions are for those that are unknown

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Sector
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="18"| 0x37
 | rowspan="18"| Play
 | rowspan="18"| Client
 | rowspan="4"| Header
 | colspan="2"| Sender
 | UUID
 | Used by the Notchian client for the disableChat launch option. Setting both longs to 0 will always display the message regardless of the setting.
 |-
 | colspan="2"| Index
 | VarInt
 | 
 |-
 | colspan="2"| Message Signature Present
 | Boolean
 | States if a message signature is present
 |-
 | colspan="2"| Message Signature bytes
 | Optional Byte Array (256)
 | Only present if <code>Message Signature Present</code> is true. Cryptography, the signature consists of the Sender UUID, Session UUID from the [[#Player Session|Player Session]] packet, Index, Salt, Timestamp in epoch seconds, the length of the original chat content, the original content itself, the length of Previous Messages, and all of the Previous message signatures. These values are hashed with [https://en.wikipedia.org/wiki/SHA-2 SHA-256] and signed using the [https://en.wikipedia.org/wiki/RSA_(cryptosystem) RSA] cryptosystem. Modifying any of these values in the packet will cause this signature to fail. This buffer is always 256 bytes long and it is not length-prefixed.
 |-
 | rowspan="3"| Body
 | colspan="2"| Message
 | String (256)
 | Raw (optionally) signed sent message content.
This is used as the <code>content</code> parameter when formatting the message on the client.
 |-
 | colspan="2"| Timestamp
 | Long
 | Represents the time the message was signed as milliseconds since the [https://en.wikipedia.org/wiki/Unix_time epoch], used to check if the message was received within 2 minutes of it being sent.
 |-
 | colspan="2"| Salt
 | Long
 | Cryptography, used for validating the message signature. 
 |-
 | rowspan="3"| Previous Messages
 | colspan="2"| Total Previous Messages
 | VarInt
 | The maximum length is 20 in Notchian client.
 |
 |-
 | rowspan="2"| Array (20)
 | Message ID
 | VarInt
 | The message Id + 1, used for validating message signature. The next field is present only when value of this field is equal to 0.
 |-
 | Signature
 | Optional Byte Array (256)
 | The previous message's signature. Contains the same type of data as <code>Message Signature bytes</code> (256 bytes) above. Not length-prefxied.
 |-
 | rowspan="4"| Other
 | colspan="2"| Unsigned Content Present
 | Boolean
 | True if the next field is present
 |-
 | colspan="2"| Unsigned Content
 | Optional Chat
 | 
 |-
 | colspan="2"| Filter Type
 | Enum VarInt
 | If the message has been filtered
 |-
 | colspan="2"| Filter Type Bits
 | Optional BitSet
 | Only present if the Filter Type is Partially Filtered. Specifies the indexes at which characters in the original message string should be replaced with the <code>#</code> symbol (i.e. filtered) by the Notchian client
 |-
 | rowspan="4"| Chat Formatting
 | colspan="2"| Chat Type
 | VarInt
 | The type of chat in the <code>minecraft:chat_type</code> registry, defined by the [[Protocol#Registry_Data|Registry Data]] packet.
 |-
 | colspan="2"| Sender Name
 | Chat
 | The name of the one sending the message, usually the sender's display name.
This is used as the <code>sender</code> parameter when formatting the message on the client.
 |-
 | colspan="2"| Has Target Name
 | Boolean
 | True if target name is present.
 |-
 | colspan="2"| Target Name
 | Optional Chat
 | The name of the one receiving the message, usually the receiver's display name. Only present if previous boolean is true.
This is used as the <code>target</code> parameter when formatting the message on the client.
 |}
[[File:MinecraftChat.drawio4.png|thumb|Player Chat Handling Logic]]

Filter Types:

The filter type mask should NOT be specified unless partially filtered is selected

{| class="wikitable"
 ! ID
 ! Name
 ! Description
 |-
 | 0
 | PASS_THROUGH
 | Message is not filtered at all
 |-
 | 1
 | FULLY_FILTERED
 | Message is fully filtered
 |-
 | 2
 | PARTIALLY_FILTERED
 | Only some characters in the message are filtered
 |}

==== End Combat ====

Unused by the Notchian client.  This data was once used for twitch.tv metadata circa 1.8.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x38
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Duration
 | VarInt
 | Length of the combat in ticks.
 |}

==== Enter Combat ====

Unused by the Notchian client.  This data was once used for twitch.tv metadata circa 1.8.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x39
 | rowspan="1"| Play
 | rowspan="1"| Client
 | colspan="3"| ''no fields''
 |}

==== Combat Death ====

Used to send a respawn screen.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x3A
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Player ID
 | VarInt
 | Entity ID of the player that died (should match the client's entity ID).
 |-
 | Message
 | Chat
 | The death message.
 |}

==== Player Info Remove ====

Used by the server to remove players from the player list.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x3B
 | rowspan="2"| Play
 | rowspan="2"| Client
 | colspan="2"| Number of Players
 | VarInt
 | Number of elements in the following array.
 |-
 | Player
 | Player Id
 | Array of UUID
 | UUIDs of players to remove.
 |}

==== Player Info Update ====

Sent by the server to update the user list (<tab> in the client).
{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="4"| 0x3C
 | rowspan="4"| Play
 | rowspan="4"| Client
 | colspan="2"| Actions
 | colspan="2"| [[#Definitions:byte|Byte]]
 | Determines what actions are present.
 |-
 | colspan="2"| Number Of Players
 | colspan="2"| [[#Definitions:varint|VarInt]]
 | Number of elements in the following array.
 |-
 | rowspan="2" | Players
 | UUID
 | rowspan="2" | Array
 | [[#Definitions:uuid|UUID]]
 | The player UUID
 |-
 | Player Actions
 | Array of [[#player-info:player-actions|Player&nbsp;Actions]]
 | The length of this array is determined by the number of [[#player-info:player-actions|Player Actions]] that give a non-zero value when applying its mask to the actions flag. For example given the decimal number 5, binary 00000101. The masks 0x01 and 0x04 would return a non-zero value, meaning the Player Actions array would include two actions: Add Player and Update Game Mode.
 |}
 

{| class="wikitable"
 |+ id="player-info:player-actions" | Player Actions
 ! Action
 ! Mask
 ! colspan="2" | Field Name
 ! colspan="2" | Type
 ! Notes
 |-
 | rowspan="6" | Add Player
 | rowspan="6" | 0x01
 | colspan="2" | Name
 | colspan="2" | [[#Definitions:string|String (16)]]
 |-
 | colspan="2" | Number Of Properties
 | colspan="2" | [[#Definitions:varint|VarInt]]
 | Number of elements in the following array.
 |-
 | rowspan="4" | Property
 | Name
 | rowspan="4"| Array
 | [[#Definitions:string|String (32767)]]
 |
 |-
 | Value
 | [[#Definitions:string|String (32767)]]
 |
 |-
 | Is Signed
 | [[#Definitions:boolean|Boolean]]
 |
 |-
 | Signature
 | Optional [[#Definitions:string|String (32767)]]
 | Only if Is Signed is true.
 |-
 | rowspan="7" | Initialize Chat
 | rowspan="7" | 0x02
 | colspan="2" | Has Signature Data
 | colspan="2" | [[#Definitions:boolean|Boolean]]
 |-
 | colspan="2" | Chat session ID
 | colspan="2" | [[#Definitions:uuid|UUID]]
 | Only sent if Has Signature Data is true.
 |-
 | colspan="2" | Public key expiry time
 | colspan="2" | [[#Definitions:long|Long]]
 | Key expiry time, as a UNIX timestamp in milliseconds. Only sent if Has Signature Data is true.
 |-
 | colspan="2" | Encoded public key size
 | colspan="2" | [[#Definitions:varint|VarInt]]
 | Size of the following array. Only sent if Has Signature Data is true. Maximum length is 512 bytes.
 |-
 | colspan="2" | Encoded public key
 | colspan="2" | [[#Definitions:byte|Byte]] Array (512)
 | The player's public key, in bytes. Only sent if Has Signature Data is true.
 |-
 | colspan="2" | Public key signature size
 | colspan="2" | [[#Definitions:varint|VarInt]]
 | Size of the following array. Only sent if Has Signature Data is true. Maximum length is 4096 bytes.
 |-
 | colspan="2" | Public key signature
 | colspan="2" | [[#Definitions:byte|Byte]] Array (4096)
 | The public key's digital signature. Only sent if Has Signature Data is true.
 |-
 | Update Game Mode
 | 0x04
 | colspan="2" | Game Mode
 | colspan="2" | [[#Definitions:varint|VarInt]]
 |-
 | Update Listed
 | 0x08
 | colspan="2" | Listed
 | colspan="2" | [[#Definitions:boolean|Boolean]]
 | Whether the player should be listed on the player list.
 |-
 | Update Latency
 | 0x10
 | colspan="2" | Ping
 | colspan="2" | [[#Definitions:varint|VarInt]]
 | Measured in milliseconds.
 |-
 | rowspan="2" | Update Display Name
 | rowspan="2" | 0x20
 | colspan="2" | Has Display Name
 | colspan="2" | [[#Definitions:boolean|Boolean]]
 |-
 | colspan="2" | Display Name
 | colspan="2" | Optional [[#Definitions:chat|Chat]]
 | Only sent if Has Display Name is true.
 |}

The Property field looks as in the response of [[Mojang API#UUID -> Profile + Skin/Cape]], except of course using the protocol format instead of JSON. That is, each player will usually have one property with Name “textures” and Value being a base64-encoded JSON string as documented at [[Mojang API#UUID -> Profile + Skin/Cape]]. An empty properties array is also acceptable, and will cause clients to display the player with one of the two default skins depending on UUID.

Ping values correspond with icons in the following way:
* A ping that negative (i.e. not known to the server yet) will result in the no connection icon.
* A ping under 150 milliseconds will result in 5 bars
* A ping under 300 milliseconds will result in 4 bars
* A ping under 600 milliseconds will result in 3 bars
* A ping under 1000 milliseconds (1 second) will result in 2 bars
* A ping greater than or equal to 1 second will result in 1 bar.

==== Look At ====

Used to rotate the client player to face the given location or entity (for <code>/teleport [<targets>] <x> <y> <z> facing</code>).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| 0x3D
 | rowspan="8"| Play
 | rowspan="8"| Client
 |-
 | Feet/eyes
 | VarInt Enum
 | Values are feet=0, eyes=1.  If set to eyes, aims using the head position; otherwise aims using the feet position.
 |-
 | Target x
 | Double
 | x coordinate of the point to face towards.
 |-
 | Target y
 | Double
 | y coordinate of the point to face towards.
 |-
 | Target z
 | Double
 | z coordinate of the point to face towards.
 |-
 | Is entity
 | Boolean
 | If true, additional information about an entity is provided.
 |-
 | Entity ID
 | Optional VarInt
 | Only if is entity is true &mdash; the entity to face towards.
 |-
 | Entity feet/eyes
 | Optional VarInt Enum
 | Whether to look at the entity's eyes or feet.  Same values and meanings as before, just for the entity's head/feet.
 |}

If the entity given by entity ID cannot be found, this packet should be treated as if is entity was false.

==== Synchronize Player Position ====

Updates the player's position on the server. This packet will also close the “Downloading Terrain” screen when joining/respawning.

If the distance between the last known position of the player on the server and the new position set by this packet is greater than 100 meters, the client will be kicked for “You moved too quickly :( (Hacking?)”.

Also if the fixed-point number of X or Z is set greater than <code>3.2E7D</code> the client will be kicked for “Illegal position”.

Yaw is measured in degrees, and does not follow classical trigonometry rules. The unit circle of yaw on the XZ-plane starts at (0, 1) and turns counterclockwise, with 90 at (-1, 0), 180 at (0, -1) and 270 at (1, 0). Additionally, yaw is not clamped to between 0 and 360 degrees; any number is valid, including negative numbers and numbers greater than 360.

Pitch is measured in degrees, where 0 is looking straight ahead, -90 is looking straight up, and 90 is looking straight down.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="7"| 0x3E
 | rowspan="7"| Play
 | rowspan="7"| Client
 | X
 | [[#Definitions:double|Double]]
 | Absolute or relative position, depending on Flags.
 |-
 | Y
 | [[#Definitions:double|Double]]
 | Absolute or relative position, depending on Flags.
 |-
 | Z
 | [[#Definitions:double|Double]]
 | Absolute or relative position, depending on Flags.
 |-
 | Yaw
 | [[#Definitions:float|Float]]
 | Absolute or relative rotation on the X axis, in degrees.
 |-
 | Pitch
 | [[#Definitions:float|Float]]
 | Absolute or relative rotation on the Y axis, in degrees.
 |-
 | Flags
 | [[#Definitions:byte|Byte]]
 | Reference the Flags table below. When the value of the this byte masked is zero the field is absolute, otherwise relative.
 |-
 | Teleport ID
 | [[#Definitions:varint|VarInt]]
 | Client should confirm this packet with [[#Confirm Teleportation|Confirm Teleportation]] containing the same Teleport ID.
 |}

{| class="wikitable" 
 |+ Flags
 |-
 ! Field
 ! Hex Mask
 |-
 | X
 | 0x01
 |-
 | Y
 | 0x02
 |-
 | Z
 | 0x04
 |-
 | Y_ROT (Pitch)
 | 0x08
 |-
 | X_ROT (Yaw)
 | 0x10
 |}

==== Update Recipe Book ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="14"| 0x3F
 | rowspan="14"| Play
 | rowspan="14"| Client
 |-
 | Action
 | VarInt
 | 0: init, 1: add, 2: remove.
 |-
 | Crafting Recipe Book Open
 | Boolean
 | If true, then the crafting recipe book will be open when the player opens its inventory.
 |-
 | Crafting Recipe Book Filter Active
 | Boolean
 | If true, then the filtering option is active when the players opens its inventory.
 |-
 | Smelting Recipe Book Open
 | Boolean
 | If true, then the smelting recipe book will be open when the player opens its inventory.
 |-
 | Smelting Recipe Book Filter Active
 | Boolean
 | If true, then the filtering option is active when the players opens its inventory.
 |-
 | Blast Furnace Recipe Book Open
 | Boolean
 | If true, then the blast furnace recipe book will be open when the player opens its inventory.
 |-
 | Blast Furnace Recipe Book Filter Active
 | Boolean
 | If true, then the filtering option is active when the players opens its inventory.
 |-
 | Smoker Recipe Book Open
 | Boolean
 | If true, then the smoker recipe book will be open when the player opens its inventory.
 |-
 | Smoker Recipe Book Filter Active
 | Boolean
 | If true, then the filtering option is active when the players opens its inventory.
 |-
 | Array size 1
 | VarInt
 | Number of elements in the following array.
 |-
 | Recipe IDs
 | Array of Identifier
 |
 |-
 | Array size 2
 | Optional VarInt
 | Number of elements in the following array, only present if mode is 0 (init).
 |-
 | Recipe IDs
 | Optional Array of Identifier
 | Only present if mode is 0 (init)
 |}
Action:
* 0 (init) = All the recipes in list 1 will be tagged as displayed, and all the recipes in list 2 will be added to the recipe book. Recipes that aren't tagged will be shown in the notification.
* 1 (add) = All the recipes in the list are added to the recipe book and their icons will be shown in the notification.
* 2 (remove) = Remove all the recipes in the list. This allows them to be re-displayed when they are re-added.

==== Remove Entities ====

Sent by the server when an entity is to be destroyed on the client.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x40
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Count
 | VarInt
 | Number of elements in the following array.
 |-
 | Entity IDs
 | Array of VarInt
 | The list of entities to destroy.
 |}

==== Remove Entity Effect ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x41
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | VarInt
 |
 |-
 | Effect ID
 | VarInt
 | See {{Minecraft Wiki|Status effect#Effect list|this table}}.
 |}

==== Reset Score ====

This is sent to the client when it should remove a scoreboard item.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x42
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Entity Name
 | String (32767)
 | The entity whose score this is. For players, this is their username; for other entities, it is their UUID.
 |-
 | Has Objective Name
 | Boolean
 | Whether the score should be removed for the specified objective, or for all of them.
 |-
 | Objective Name
 | Optional String (32767)
 | The name of the objective the score belongs to. Only present if the previous field is true.
 |}

==== Remove Resource Pack (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x43
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Has UUID
 | Boolean
 | Whether a specific resource pack should be removed, or all of them.
 |-
 | UUID
 | Optional UUID
 | The UUID of the resource pack to be removed. Only present if the previous field is true.
 |}

==== Add Resource Pack (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="6"| 0x44
 | rowspan="6"| Play
 | rowspan="6"| Client
 | UUID
 | UUID
 | The unique identifier of the resource pack.
 |-
 | URL
 | String (32767)
 | The URL to the resource pack.
 |-
 | Hash
 | String (40)
 | A 40 character hexadecimal, case-insensitive [[wikipedia:SHA-1|SHA-1]] hash of the resource pack file.<br />If it's not a 40 character hexadecimal string, the client will not use it for hash verification and likely waste bandwidth.
 |-
 | Forced
 | Boolean
 | The Notchian client will be forced to use the resource pack from the server. If they decline they will be kicked from the server.
 |-
 | Has Prompt Message
 | Boolean
 | Whether a custom message should be used on the resource pack prompt.
 |-
 | Prompt Message
 | Optional Chat
 | This is shown in the prompt making the client accept or decline the resource pack. Only present if the previous field is true.
 |}

==== Respawn ====

{{Need Info|Although the number of portal cooldown ticks is included in this packet, the whole portal usage process is still dictated entirely by the server. What kind of effect does this value have on the client, if any?}}

To change the player's dimension (overworld/nether/end), send them a respawn packet with the appropriate dimension, followed by prechunks/chunks for the new dimension, and finally a position and look packet. You do not need to unload chunks, the client will do it automatically.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="12"| 0x45
 | rowspan="12"| Play
 | rowspan="12"| Client
 | Dimension Type
 | Identifier
 | The type of dimension in the <code>minecraft:dimension_type</code> registry, defined by the [[Protocol#Registry_Data|Registry Data]] packet.
 |-
 | Dimension Name
 | Identifier
 | Name of the dimension being spawned into.
 |-
 | Hashed seed
 | Long
 | First 8 bytes of the SHA-256 hash of the world's seed. Used client side for biome noise
 |-
 | Game mode
 | Unsigned Byte
 | 0: Survival, 1: Creative, 2: Adventure, 3: Spectator.
 |-
 | Previous Game mode
 | Byte
 | -1: Undefined (null), 0: Survival, 1: Creative, 2: Adventure, 3: Spectator. The previous game mode. Vanilla client uses this for the debug (F3 + N & F3 + F4) game mode switch. (More information needed)
 |-
 | Is Debug
 | Boolean
 | True if the world is a {{Minecraft Wiki|debug mode}} world; debug mode worlds cannot be modified and have predefined blocks.
 |-
 | Is Flat
 | Boolean
 | True if the world is a {{Minecraft Wiki|superflat}} world; flat worlds have different void fog and a horizon at y=0 instead of y=63.
 |-
 | Has death location
 | Boolean
 | If true, then the next two fields are present.
 |-
 | Death dimension Name
 | Optional Identifier
 | Name of the dimension the player died in.
 |-
 | Death location
 | Optional Position
 | The location that the player died at.
 |-
 | Portal cooldown
 | VarInt
 | The number of ticks until the player can use the portal again.
 |-
 | Data kept
 | Byte
 | Bit mask. 0x01: Keep attributes, 0x02: Keep metadata. Tells which data should be kept on the client side once the player has respawned.
In the Notchian implementation, this is context dependent:
* normal respawns (after death) keep no data;
* exiting the end poem/credits keeps the attributes;
* other dimension changes (portals or teleports) keep all data.
 |}

{{Warning2|Avoid changing player's dimension to same dimension they were already in unless they are dead. If you change the dimension to one they are already in, weird bugs can occur, such as the player being unable to attack other players in new world (until they die and respawn).

Before 1.16, if you must respawn a player in the same dimension without killing them, send two respawn packets, one to a different world and then another to the world you want. You do not need to complete the first respawn; it only matters that you send two packets.}}

==== Set Head Rotation ====

Changes the direction an entity's head is facing.

While sending the Entity Look packet changes the vertical rotation of the head, sending this packet appears to be necessary to rotate the head horizontally.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x46
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | VarInt
 |
 |-
 | Head Yaw
 | Angle
 | New angle, not a delta.
 |}

==== Update Section Blocks ====

Fired whenever 2 or more blocks are changed within the same chunk on the same tick.

{{Warning|Changing blocks in chunks not loaded by the client is unsafe (see note on [[#Block Update|Block Update]]).}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x47
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Chunk section position
 | Long
 | Chunk section coordinate (encoded chunk x and z with each 22 bits, and section y with 20 bits, from left to right).
 |-
 | Blocks array size
 | VarInt
 | Number of elements in the following array.
 |-
 | Blocks
 | Array of VarLong
 | Each entry is composed of the block state id, shifted left by 12, and the relative block position in the chunk section (4 bits for x, z, and y, from left to right).
 |}

Chunk section position is encoded:
<syntaxhighlight lang="java">
((sectionX & 0x3FFFFF) << 42) | (sectionY & 0xFFFFF) | ((sectionZ & 0x3FFFFF) << 20);
</syntaxhighlight>
and decoded:
<syntaxhighlight lang="java">
sectionX = long >> 42;
sectionY = long << 44 >> 44;
sectionZ = long << 22 >> 42;
</syntaxhighlight>

Blocks are encoded:
<syntaxhighlight lang="java">
blockStateId << 12 | (blockLocalX << 8 | blockLocalZ << 4 | blockLocalY)
//Uses the local position of the given block position relative to its respective chunk section
</syntaxhighlight>
and decoded:
<syntaxhighlight lang="java">
blockStateId = long >> 12;
blockLocalX = (long >> 8) & 0xF;
blockLocalY = long & 0xF;
blockLocalZ = (long >> 4) & 0xF;
</syntaxhighlight>

==== Select Advancements Tab ====

Sent by the server to indicate that the client should switch advancement tab. Sent either when the client switches tab in the GUI or when an advancement in another tab is made.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x48
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Has id
 | Boolean
 | Indicates if the next field is present.
 |-
 | Optional Identifier
 | Identifier
 | See below.
 |}

The Identifier can be one of the following:

{| class="wikitable"
 ! Optional Identifier
 |-
 | minecraft:story/root
 |-
 | minecraft:nether/root
 |-
 | minecraft:end/root
 |-
 | minecraft:adventure/root
 |-
 | minecraft:husbandry/root
 |}

If no or an invalid identifier is sent, the client will switch to the first tab in the GUI.

==== Server Data ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x49
 | rowspan="5"| Play
 | rowspan="5"| Client
 | MOTD
 | [[#Definitions:chat|Chat]]
 |
 |-
 | Has Icon
 | [[#Definitions:boolean|Boolean]]
 |
 |-
 | Size
 | VarInt
 | Number of bytes in the following array.
 |-
 | Icon
 | Optional [[#Definitions:byte-array|Byte Array]]
 | Icon bytes in the PNG format
 |-
 | Enforces Secure Chat
 | [[#Definitions:boolean|Boolean]]
 |
 |}

==== Set Action Bar Text ====

Displays a message above the hotbar. Equivalent to [[#System Chat Message|System Chat Message]] with Overlay set to true, except that [[Chat#Social Interactions (blocking)|chat message blocking]] isn't performed. Used by the Notchian server only to implement the <code>/title</code> command.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x4A
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Action bar text
 | Chat
 |}

==== Set Border Center ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x4B
 | rowspan="2"| Play
 | rowspan="2"| Client
 | X
 | Double
 |
 |-
 | Z
 | Double
 |
 |}

==== Set Border Lerp Size ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x4C
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Old Diameter
 | Double
 | Current length of a single side of the world border, in meters.
 |-
 | New Diameter
 | Double
 | Target length of a single side of the world border, in meters.
 |-
 | Speed
 | VarLong
 | Number of real-time ''milli''seconds until New Diameter is reached. It appears that Notchian server does not sync world border speed to game ticks, so it gets out of sync with server lag. If the world border is not moving, this is set to 0.
 |}

==== Set Border Size ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x4D
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Diameter
 | Double
 | Length of a single side of the world border, in meters.
 |}

==== Set Border Warning Delay ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x4E
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Warning Time
 | VarInt
 | In seconds as set by <code>/worldborder warning time</code>.
 |}

==== Set Border Warning Distance ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x4F
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Warning Blocks
 | VarInt
 | In meters.
 |}

==== Set Camera ====

Sets the entity that the player renders from. This is normally used when the player left-clicks an entity while in spectator mode.

The player's camera will move with the entity and look where it is looking. The entity is often another player, but can be any type of entity.  The player is unable to move this entity (move packets will act as if they are coming from the other entity).

If the given entity is not loaded by the player, this packet is ignored.  To return control to the player, send this packet with their entity ID.

The Notchian server resets this (sends it back to the default entity) whenever the spectated entity is killed or the player sneaks, but only if they were spectating an entity. It also sends this packet whenever the player switches out of spectator mode (even if they weren't spectating an entity).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x50
 | Play
 | Client
 | Camera ID
 | VarInt
 | ID of the entity to set the client's camera to.
 |}

The notchian client also loads certain shaders for given entities:

* Creeper &rarr; <code>shaders/post/creeper.json</code>
* Spider (and cave spider) &rarr; <code>shaders/post/spider.json</code>
* Enderman &rarr; <code>shaders/post/invert.json</code>
* Anything else &rarr; the current shader is unloaded

==== Set Held Item ====

Sent to change the player's slot selection.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x51
 | Play
 | Client
 | Slot
 | Byte
 | The slot which the player has selected (0–8).
 |}

==== Set Center Chunk ====

{{Need Info|Why is this even needed?  Is there a better name for it?  My guess is that it's something to do with logical behavior with latency, but it still seems weird.}}

Updates the client's location.  This is used to determine what chunks should remain loaded and if a chunk load should be ignored; chunks outside of the view distance may be unloaded.

Sent whenever the player moves across a chunk border horizontally, and also (according to testing) for any integer change in the vertical axis, even if it doesn't go across a chunk section border.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x52
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Chunk X
 | VarInt
 | Chunk X coordinate of the player's position.
 |-
 | Chunk Z
 | VarInt
 | Chunk Z coordinate of the player's position.
 |}

==== Set Render Distance ====

Sent by the integrated singleplayer server when changing render distance.  This packet is sent by the server when the client reappears in the overworld after leaving the end.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x53
 | Play
 | Client
 | View Distance
 | VarInt
 | Render distance (2-32).
 |}

==== Set Default Spawn Position ====

Sent by the server after login to specify the coordinates of the spawn point (the point at which players spawn at, and which the compass points to). It can be sent at any time to update the point compasses point at.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x54
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Location
 | Position
 | Spawn location.
 |-
 | Angle
 | Float
 | The angle at which to respawn at.
 |}

==== Display Objective ====

This is sent to the client when it should display a scoreboard.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x55
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Position
 | VarInt
 | The position of the scoreboard. 0: list, 1: sidebar, 2: below name, 3 - 18: team specific sidebar, indexed as 3 + team color.
 |-
 | Score Name
 | String (32767)
 | The unique name for the scoreboard to be displayed.
 |}

==== Set Entity Metadata ====

Updates one or more [[Entity_metadata#Entity Metadata Format|metadata]] properties for an existing entity. Any properties not included in the Metadata field are left unchanged.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x56
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Entity ID
 | VarInt
 |
 |-
 | Metadata
 | [[Entity_metadata#Entity Metadata Format|Entity Metadata]]
 |
 |}

==== Link Entities ====

This packet is sent when an entity has been {{Minecraft Wiki|Lead|leashed}} to another entity.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x57
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Attached Entity ID
 | Int
 | Attached entity's EID.
 |-
 | Holding Entity ID
 | Int
 | ID of the entity holding the lead. Set to -1 to detach.
 |}

==== Set Entity Velocity ====

Velocity is in units of 1/8000 of a block per server tick (50ms); for example, -1343 would move (-1343 / 8000) = −0.167875 blocks per tick (or −3.3575 blocks per second).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| 0x58
 | rowspan="4"| Play
 | rowspan="4"| Client
 | Entity ID
 | VarInt
 |
 |-
 | Velocity X
 | Short
 | Velocity on the X axis.
 |-
 | Velocity Y
 | Short
 | Velocity on the Y axis.
 |-
 | Velocity Z
 | Short
 | Velocity on the Z axis.
 |}

==== Set Equipment ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| 0x59
 | rowspan="3"| Play
 | rowspan="3"| Client
 | colspan="2"| Entity ID
 | colspan="2"| VarInt
 | Entity's ID.
 |-
 | rowspan="2"| Equipment
 | Slot
 | rowspan="2"| Array
 | Byte Enum
 | Equipment slot. 0: main hand, 1: off hand, 2–5: armor slot (2: boots, 3: leggings, 4: chestplate, 5: helmet).  Also has the top bit set if another entry follows, and otherwise unset if this is the last item in the array.
 |-
 | Item
 | [[Slot Data|Slot]]
 |
 |}

==== Set Experience ====

Sent by the server when the client should change experience levels.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x5A
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Experience bar
 | Float
 | Between 0 and 1.
 |-
 | Level
 | VarInt
 |
 |-
 | Total Experience
 | VarInt
 | See {{Minecraft Wiki|Experience#Leveling up}} on the Minecraft Wiki for Total Experience to Level conversion.
 |}

==== Set Health ====

Sent by the server to set the health of the player it is sent to.

Food {{Minecraft Wiki|Food#Hunger vs. Saturation|saturation}} acts as a food “overcharge”. Food values will not decrease while the saturation is over zero. New players logging in or respawning automatically get a saturation of 5.0. Eating food increases the saturation as well as the food bar.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x5B
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Health
 | Float
 | 0 or less = dead, 20 = full HP.
 |-
 | Food
 | VarInt
 | 0–20.
 |-
 | Food Saturation
 | Float
 | Seems to vary from 0.0 to 5.0 in integer increments.
 |}

==== Update Objectives ====

This is sent to the client when it should create a new {{Minecraft Wiki|scoreboard}} objective or remove one.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="10"| 0x5C
 | rowspan="10"| Play
 | rowspan="10"| Client
 | colspan="2"| Objective Name
 | String (32767)
 | A unique name for the objective.
 |-
 | colspan="2"| Mode
 | Byte
 | 0 to create the scoreboard. 1 to remove the scoreboard. 2 to update the display text.
 |-
 | colspan="2"| Objective Value
 | Optional Chat
 | Only if mode is 0 or 2.The text to be displayed for the score.
 |-
 | colspan="2"| Type
 | Optional VarInt Enum
 | Only if mode is 0 or 2. 0 = "integer", 1 = "hearts".
 |-
 | colspan="2"| Has Number Format
 | Optional Boolean
 | Only if mode is 0 or 2. Whether this objective has a set number format for the scores.
 |-
 | colspan="2"| Number Format
 | Optional VarInt Enum
 | Only if mode is 0 or 2 and the previous boolean is true. Determines how the score number should be formatted.
 |-
 ! Number Format
 ! Field Name
 !
 !
 |-
 | 0: blank
 | colspan="2"| ''no fields''
 | Show nothing.
 |-
 | 1: styled
 | Styling
 | [[NBT#Specification:compound_tag|Compound Tag]]
 | The styling to be used when formatting the score number. Contains the [[Text formatting#Styling fields|text component styling fields]].
 |-
 | 2: fixed
 | Content
 | Chat
 | The text to be used as placeholder.
 |}

==== Set Passengers ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x5D
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Entity ID
 | VarInt
 | Vehicle's EID.
 |-
 | Passenger Count
 | VarInt
 | Number of elements in the following array.
 |-
 | Passengers
 | Array of VarInt
 | EIDs of entity's passengers.
 |}

==== Update Teams ====

Creates and updates teams.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="23"| 0x5E
 | rowspan="23"| Play
 | rowspan="23"| Client
 | colspan="2"| Team Name
 | String (32767)
 | A unique name for the team. (Shared with scoreboard).
 |-
 | colspan="2"| Mode
 | Byte
 | Determines the layout of the remaining packet.
 |-
 | rowspan="9"| 0: create team
 | Team Display Name
 | Chat
 |
 |-
 | Friendly Flags
 | Byte
 | Bit mask. 0x01: Allow friendly fire, 0x02: can see invisible players on same team.
 |-
 | Name Tag Visibility
 | String Enum (40)
 | <code>always</code>, <code>hideForOtherTeams</code>, <code>hideForOwnTeam</code>, <code>never</code>.
 |-
 | Collision Rule
 | String Enum (40)
 | <code>always</code>, <code>pushOtherTeams</code>, <code>pushOwnTeam</code>, <code>never</code>.
 |-
 | Team Color
 | VarInt Enum
 | Used to color the name of players on the team; see below.
 |-
 | Team Prefix
 | Chat
 | Displayed before the names of players that are part of this team.
 |-
 | Team Suffix
 | Chat
 | Displayed after the names of players that are part of this team.
 |-
 | Entity Count
 | VarInt
 | Number of elements in the following array.
 |-
 | Entities
 | Array of String (32767)
 | Identifiers for the entities in this team.  For players, this is their username; for other entities, it is their UUID.
 |-
 | 1: remove team
 | ''no fields''
 | ''no fields''
 |
 |-
 | rowspan="7"| 2: update team info
 | Team Display Name
 | Chat
 |
 |-
 | Friendly Flags
 | Byte
 | Bit mask. 0x01: Allow friendly fire, 0x02: can see invisible entities on same team.
 |-
 | Name Tag Visibility
 | String Enum (40)
 | <code>always</code>, <code>hideForOtherTeams</code>, <code>hideForOwnTeam</code>, <code>never</code>
 |-
 | Collision Rule
 | String Enum (40)
 | <code>always</code>, <code>pushOtherTeams</code>, <code>pushOwnTeam</code>, <code>never</code>
 |-
 | Team Color
 | VarInt Enum
 | Used to color the name of players on the team; see below.
 |-
 | Team Prefix
 | Chat
 | Displayed before the names of players that are part of this team.
 |-
 | Team Suffix
 | Chat
 | Displayed after the names of players that are part of this team.
 |-
 | rowspan="2"| 3: add entities to team
 | Entity Count
 | VarInt
 | Number of elements in the following array.
 |-
 | Entities
 | Array of String (32767)
 | Identifiers for the added entities.  For players, this is their username; for other entities, it is their UUID.
 |-
 | rowspan="2"| 4: remove entities from team
 | Entity Count
 | VarInt
 | Number of elements in the following array.
 |-
 | Entities
 | Array of String (32767)
 | Identifiers for the removed entities.  For players, this is their username; for other entities, it is their UUID.
 |}

Team Color: The color of a team defines how the names of the team members are visualized; any formatting code can be used. The following table lists all the possible values.

{| class="wikitable"
 ! ID
 ! Formatting
 |-
 | 0-15
 | Color formatting, same values as in [[Text formatting#Colors]].
 |-
 | 16
 | Obfuscated
 |-
 | 17
 | Bold
 |-
 | 18
 | Strikethrough
 |-
 | 19
 | Underlined
 |-
 | 20
 | Italic
 |-
 | 21
 | Reset
 |}

==== Update Score ====

This is sent to the client when it should update a scoreboard item.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="11"| 0x5F
 | rowspan="11"| Play
 | rowspan="11"| Client
 | colspan="2"| Entity Name
 | String (32767)
 | The entity whose score this is. For players, this is their username; for other entities, it is their UUID.
 |-
 | colspan="2"| Objective Name
 | String (32767)
 | The name of the objective the score belongs to.
 |-
 | colspan="2"| Value
 | VarInt
 | The score to be displayed next to the entry.
 |-
 | colspan="2"| Has Display Name
 | Boolean
 | Whether this score has a custom display name.
 |-
 | colspan="2"| Display Name
 | Optional Chat
 | The custom display name. Only present if the previous boolean is true.
 |-
 | colspan="2"| Has Number Format
 | Boolean
 | Whether this score has a set number format. This overrides the number format set on the objective, if any.
 |-
 | colspan="2"| Number Format
 | Optional VarInt Enum
 | Determines how the score number should be formatted. Only present if the previous boolean is true.
 |-
 ! Number Format
 ! Field Name
 !
 !
 |-
 | 0: blank
 | colspan="2"| ''no fields''
 | Show nothing.
 |-
 | 1: styled
 | Styling
 | [[NBT#Specification:compound_tag|Compound Tag]]
 | The styling to be used when formatting the score number. Contains the [[Text formatting#Styling fields|text component styling fields]].
 |-
 | 2: fixed
 | Content
 | Chat
 | The text to be used as placeholder.
 |}

==== Set Simulation Distance ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x60
 | Play
 | Client
 | Simulation Distance
 | VarInt
 | The distance that the client will process specific things, such as entities.
 |}

==== Set Subtitle Text ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x61
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Subtitle Text
 | Chat
 |
 |}

==== Update Time ====

Time is based on ticks, where 20 ticks happen every second. There are 24000 ticks in a day, making Minecraft days exactly 20 minutes long.

The time of day is based on the timestamp modulo 24000. 0 is sunrise, 6000 is noon, 12000 is sunset, and 18000 is midnight.

The default SMP server increments the time by <code>20</code> every second.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x62
 | rowspan="2"| Play
 | rowspan="2"| Client
 | World Age
 | Long
 | In ticks; not changed by server commands.
 |-
 | Time of day
 | Long
 | The world (or region) time, in ticks. If negative the sun will stop moving at the Math.abs of the time.
 |}

==== Set Title Text ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x63
 | rowspan="1"| Play
 | rowspan="1"| Client
 | Title Text
 | Chat
 |
 |}

==== Set Title Animation Times ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x64
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Fade In
 | Int
 | Ticks to spend fading in.
 |-
 | Stay
 | Int
 | Ticks to keep the title displayed.
 |-
 | Fade Out
 | Int
 | Ticks to spend fading out, not when to start fading out.
 |}

==== Entity Sound Effect ====

Plays a sound effect from an entity, either by hardcoded ID or Identifier. Sound IDs and names can be found [https://pokechu22.github.io/Burger/1.20.4.html#sounds here].

{{Warning|Numeric sound effect IDs are liable to change between versions}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="9"| 0x65
 | rowspan="9"| Play
 | rowspan="9"| Client
 | Sound ID
 | VarInt
 | Represents the <code>Sound ID + 1</code>. If the value is 0, the packet contains a sound specified by Identifier.
 |-
 | Sound Name
 | Optional Identifier
 | Only present if Sound ID is 0
 |-
 | Has Fixed Range
 | Optional Boolean
 | Only present if Sound ID is 0.
 |-
 | Range
 | Optional Float
 | The fixed range of the sound. Only present if previous boolean is true and Sound ID is 0.
 |-
 | Sound Category
 | VarInt Enum
 | The category that this sound will be played from ([https://gist.github.com/konwboj/7c0c380d3923443e9d55 current categories]).
 |-
 | Entity ID
 | VarInt
 |
 |-
 | Volume
 | Float
 | 1.0 is 100%, capped between 0.0 and 1.0 by Notchian clients.
 |-
 | Pitch
 | Float
 | Float between 0.5 and 2.0 by Notchian clients.
 |-
 | Seed
 | Long
 | Seed used to pick sound variant.
 |}

==== Sound Effect ====

Plays a sound effect at the given location, either by hardcoded ID or Identifier. Sound IDs and names can be found [https://pokechu22.github.io/Burger/1.20.4.html#sounds here].

{{Warning|Numeric sound effect IDs are liable to change between versions}}

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="11"| 0x66
 | rowspan="11"| Play
 | rowspan="11"| Client
 | Sound ID
 | VarInt
 | Represents the <code>Sound ID + 1</code>. If the value is 0, the packet contains a sound specified by Identifier.
 |-
 | Sound Name
 | Optional Identifier
 | Only present if Sound ID is 0
 |-
 | Has Fixed Range
 | Optional Boolean
 | Only present if Sound ID is 0.
 |-
 | Range
 | Optional Float
 | The fixed range of the sound. Only present if previous boolean is true and Sound ID is 0.
 |-
 | Sound Category
 | VarInt Enum
 | The category that this sound will be played from ([https://gist.github.com/konwboj/7c0c380d3923443e9d55 current categories]).
 |-
 | Effect Position X
 | Int
 | Effect X multiplied by 8 ([[Data types#Fixed-point numbers|fixed-point number]] with only 3 bits dedicated to the fractional part).
 |-
 | Effect Position Y
 | Int
 | Effect Y multiplied by 8 ([[Data types#Fixed-point numbers|fixed-point number]] with only 3 bits dedicated to the fractional part).
 |-
 | Effect Position Z
 | Int
 | Effect Z multiplied by 8 ([[Data types#Fixed-point numbers|fixed-point number]] with only 3 bits dedicated to the fractional part).
 |-
 | Volume
 | Float
 | 1.0 is 100%, capped between 0.0 and 1.0 by Notchian clients.
 |-
 | Pitch
 | Float
 | Float between 0.5 and 2.0 by Notchian clients.
 |-
 | Seed
 | Long
 | Seed used to pick sound variant.
 |}

==== Start Configuration ====

Sent during gameplay in order to redo the configuration process. The client must respond with [[#Acknowledge Configuration|Acknowledge Configuration]] for the process to start.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x67
 | rowspan="1"| Play
 | rowspan="1"| Client
 | colspan="3"| ''no fields''
 |}

This packet switches the connection state to [[#Configuration|configuration]].

==== Stop Sound ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x68
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Flags
 | Byte
 | Controls which fields are present.
 |-
 | Source
 | Optional VarInt Enum
 | Only if flags is 3 or 1 (bit mask 0x1). See below. If not present, then sounds from all sources are cleared.
 |-
 | Sound
 | Optional Identifier
 | Only if flags is 2 or 3 (bit mask 0x2).  A sound effect name, see [[#Custom Sound Effect|Custom Sound Effect]]. If not present, then all sounds are cleared.
 |}

Categories:

{| class="wikitable"
 ! Name !! Value
 |-
 | master || 0
 |-
 | music || 1
 |-
 | record || 2
 |-
 | weather || 3
 |-
 | block || 4
 |-
 | hostile || 5
 |-
 | neutral || 6
 |-
 | player || 7
 |-
 | ambient || 8
 |-
 | voice || 9
 |}

==== System Chat Message ====

Sends the client a raw system message.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x69
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Content
 | Chat
 | Limited to 262144 bytes.
 |-
 | Overlay
 | Boolean
 | Whether the message is an actionbar or chat message. See also [[#Set Action Bar Text]].
 |}

==== Set Tab List Header And Footer ====

This packet may be used by custom servers to display additional information above/below the player list. It is never sent by the Notchian server.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x6A
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Header
 | Chat
 | To remove the header, send a empty text component: <code>{"text":""}</code>.
 |-
 | Footer
 | Chat
 | To remove the footer, send a empty text component: <code>{"text":""}</code>.
 |}

==== Tag Query Response ====

Sent in response to [[#Query Block Entity Tag|Query Block Entity Tag]] or [[#Query Entity Tag|Query Entity Tag]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x6B
 | rowspan="2"| Play
 | rowspan="2"| Client
 | Transaction ID
 | VarInt
 | Can be compared to the one sent in the original query packet.
 |-
 | NBT
 | [[NBT|NBT Tag]]
 | The NBT of the block or entity.  May be a TAG_END (0) in which case no NBT is present.
 |}

==== Pickup Item ====

Sent by the server when someone picks up an item lying on the ground — its sole purpose appears to be the animation of the item flying towards you. It doesn't destroy the entity in the client memory, and it doesn't add it to your inventory. The server only checks for items to be picked up after each [[#Set Player Position|Set Player Position]] (and [[#Set Player Position And Rotation|Set Player Position And Rotation]]) packet sent by the client. The collector entity can be any entity; it does not have to be a player. The collected entity also can be any entity, but the Notchian server only uses this for items, experience orbs, and the different varieties of arrows.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x6C
 | rowspan="3"| Play
 | rowspan="3"| Client
 | Collected Entity ID
 | VarInt
 |
 |-
 | Collector Entity ID
 | VarInt
 |
 |-
 | Pickup Item Count
 | VarInt
 | Seems to be 1 for XP orbs, otherwise the number of items in the stack.
 |}

==== Teleport Entity ====

This packet is sent by the server when an entity moves more than 8 blocks.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="7"| 0x6D
 | rowspan="7"| Play
 | rowspan="7"| Client
 | Entity ID
 | VarInt
 |
 |-
 | X
 | Double
 |
 |-
 | Y
 | Double
 |
 |-
 | Z
 | Double
 |
 |-
 | Yaw
 | Angle
 | (Y Rot)New angle, not a delta.
 |-
 | Pitch
 | Angle
 | (X Rot)New angle, not a delta.
 |-
 | On Ground
 | Boolean
 |
 |}

==== Set Ticking State ====

Used to adjust the ticking rate of the client, and whether it's frozen.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2" | 0x6E
 | rowspan="2" | Play
 | rowspan="2" | Client
 | Tick rate
 | Float
 |
 |-
 | Is frozen
 | Boolean
 |
 |}

==== Step Tick ====

Advances the client processing by the specified number of ticks. Has no effect unless client ticking is frozen.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x6F
 | Play
 | Client
 | Tick steps
 | VarInt
 |
 |}

==== Update Advancements ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="9"| 0x70
 | rowspan="9"| Play
 | rowspan="9"| Client
 | colspan="2"| Reset/Clear
 | colspan="2"| Boolean
 | Whether to reset/clear the current advancements.
 |-
 | colspan="2"| Mapping size
 | colspan="2"| VarInt
 | Size of the following array.
 |-
 | rowspan="2"| Advancement mapping
 | Key
 | rowspan="2"| Array
 | Identifier
 | The identifier of the advancement.
 |-
 | Value
 | Advancement
 | See below
 |-
 | colspan="2"| List size
 | colspan="2"| VarInt
 | Size of the following array.
 |-
 | colspan="2"| Identifiers
 | colspan="2"| Array of Identifier
 | The identifiers of the advancements that should be removed.
 |-
 | colspan="2"| Progress size
 | colspan="2"| VarInt
 | Size of the following array.
 |-
 | rowspan="2"| Progress mapping
 | Key
 | rowspan="2"| Array
 | Identifier
 | The identifier of the advancement.
 |-
 | Value
 | Advancement progress
 | See below.
 |}

Advancement structure:

{| class="wikitable"
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | colspan="2"| Has parent
 | colspan="2"| Boolean
 | Indicates whether the next field exists.
 |-
 | colspan="2"| Parent id
 | colspan="2"| Optional Identifier
 | The identifier of the parent advancement.
 |-
 | colspan="2"| Has display
 | colspan="2"| Boolean
 | Indicates whether the next field exists.
 |-
 | colspan="2"| Display data
 | colspan="2"| Optional advancement display
 | See below.
 |-
 | colspan="2"| Array length
 | colspan="2"| VarInt
 | Number of arrays in the following array.
 |-
 | rowspan="2"| Requirements
 | Array length 2
 | rowspan="2"| Array
 | VarInt
 | Number of elements in the following array.
 |-
 | Requirement
 | Array of String (32767)
 | Array of required criteria.
 |-
 | colspan="2"| Sends telemetry data
 | colspan="2"| Boolean
 | Whether the client should include this achievement in the telemetry data when it's completed.
The Notchian client only sends data for advancements on the <code>minecraft</code> namespace.
 |}

Advancement display:

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | Title
 | Chat
 |
 |-
 | Description
 | Chat
 |
 |-
 | Icon
 | [[Slot]]
 |
 |-
 | Frame type
 | VarInt Enum
 | 0 = <code>task</code>, 1 = <code>challenge</code>, 2 = <code>goal</code>.
 |-
 | Flags
 | Int
 | 0x01: has background texture; 0x02: <code>show_toast</code>; 0x04: <code>hidden</code>.
 |-
 | Background texture
 | Optional Identifier
 | Background texture location.  Only if flags indicates it.
 |-
 | X coord
 | Float
 |
 |-
 | Y coord
 | Float
 |
 |}

Advancement progress:

{| class="wikitable"
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | colspan="2"| Size
 | colspan="2"| VarInt
 | Size of the following array.
 |-
 | rowspan="2"| Criteria
 | Criterion identifier
 | rowspan="2"| Array
 | Identifier
 | The identifier of the criterion.
 |-
 | Criterion progress
 | Criterion progress
 |
 |}

Criterion progress:

{| class="wikitable"
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | Achieved
 | Boolean
 | If true, next field is present.
 |-
 | Date of achieving
 | Optional Long
 | As returned by [https://docs.oracle.com/javase/6/docs/api/java/util/Date.html#getTime() <code>Date.getTime</code>].
 |}

==== Update Attributes ====

Sets {{Minecraft Wiki|Attribute|attributes}} on the given entity.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="6"| 0x71
 | rowspan="6"| Play
 | rowspan="6"| Client
 | colspan="2"| Entity ID
 | colspan="2"| VarInt
 |
 |-
 | colspan="2"| Number Of Properties
 | colspan="2"| VarInt
 | Number of elements in the following array.
 |-
 | rowspan="4"| Property
 | Key
 | rowspan="4"| Array
 | Identifier
 | See below.
 |-
 | Value
 | Double
 | See below.
 |-
 | Number Of Modifiers
 | VarInt
 | Number of elements in the following array.
 |-
 | Modifiers
 | Array of Modifier Data
 | See {{Minecraft Wiki|Attribute#Modifiers}}. Modifier Data defined below.
 |}

Known Key values (see also {{Minecraft Wiki|Attribute#Modifiers}}):

{| class="wikitable"
 |-
 ! Key
 ! Default
 ! Min
 ! Max
 ! Label
 |-
 | generic.max_health
 | 20.0
 | 1.0
 | 1024.0
 | Max Health.
 |-
 | generic.follow_range
 | 32.0
 | 0.0
 | 2048.0
 | Follow Range.
 |-
 | generic.knockback_resistance
 | 0.0
 | 0.0
 | 1.0
 | Knockback Resistance.
 |-
 | generic.movement_speed
 | 0.7
 | 0.0
 | 1024.0
 | Movement Speed.
 |-
 | generic.flying_speed
 | 0.4
 | 0.0
 | 1024.0
 | Flying Speed.
 |-
 | generic.attack_damage
 | 2.0
 | 0.0
 | 2048.0
 | Attack Damage.
 |-
 | generic.attack_knockback
 | 0.0
 | 0.0
 | 5.0
 | &mdash;
 |-
 | generic.attack_speed
 | 4.0
 | 0.0
 | 1024.0
 | Attack Speed.
 |-
 | generic.armor
 | 0.0
 | 0.0
 | 30.0
 | Armor.
 |-
 | generic.armor_toughness
 | 0.0
 | 0.0
 | 20.0
 | Armor Toughness.
 |-
 | generic.luck
 | 0.0
 | -1024.0
 | 1024.0
 | Luck.
 |-
 | zombie.spawn_reinforcements
 | 0.0
 | 0.0
 | 1.0
 | Spawn Reinforcements Chance.
 |-
 | horse.jump_strength
 | 0.7
 | 0.0
 | 2.0
 | Jump Strength.
 |-
 | generic.reachDistance
 | 5.0
 | 0.0
 | 1024.0
 | Player Reach Distance (Forge only).
 |-
 | forge.swimSpeed
 | 1.0
 | 0.0
 | 1024.0
 | Swimming Speed (Forge only).
 |}

''Modifier Data'' structure:

{| class="wikitable"
 |-
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | UUID
 | UUID
 |
 |-
 | Amount
 | Double
 | May be positive or negative.
 |-
 | Operation
 | Byte
 | See below.
 |}

The operation controls how the base value of the modifier is changed.

* 0: Add/subtract amount
* 1: Add/subtract amount percent of the current value
* 2: Multiply by amount percent

All of the 0's are applied first, and then the 1's, and then the 2's.

==== Entity Effect ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="7"| 0x72
 | rowspan="7"| Play
 | rowspan="7"| Client
 | Entity ID
 | VarInt
 |
 |-
 | Effect ID
 | VarInt
 | See {{Minecraft Wiki|Status effect#Effect list|this table}}.
 |-
 | Amplifier
 | Byte
 | Notchian client displays effect level as Amplifier + 1.
 |-
 | Duration
 | VarInt
 | Duration in ticks. (-1 for infinite)
 |-
 | Flags
 | Byte
 | Bit field, see below.
 |-
 | Has Factor Data
 | Boolean
 | Used in DARKNESS effect
 |-
 | Factor Codec
 | NBT Tag
 | See below
 |}

Within flags:

* 0x01: Is ambient - was the effect spawned from a beacon?  All beacon-generated effects are ambient.  Ambient effects use a different icon in the HUD (blue border rather than gray).  If all effects on an entity are ambient, the [[Entity_metadata#Living Entity|"Is potion effect ambient" living metadata field]] should be set to true.  Usually should not be enabled.
* 0x02: Show particles - should all particles from this effect be hidden?  Effects with particles hidden are not included in the calculation of the effect color, and are not rendered on the HUD (but are still rendered within the inventory).  Usually should be enabled.
* 0x04: Show icon - should the icon be displayed on the client?  Usually should be enabled.

Factor Data
{| class="wikitable"
 !Name
 !Type
 !style="width: 250px;" colspan="2"| Notes
 |-
 | padding_duration
 | TAG_INT
 |
 |-
 | factor_start
 | TAG_FLOAT
 |
 |-
 | factor_target
 | TAG_FLOAT
 |
 |-
 | factor_current
 | TAG_FLOAT
 |
 |-
 | effect_changed_timestamp
 | TAG_INT
 |-
 | factor_previous_frame
 | TAG_FLOAT
 |-
 | had_effect_last_tick
 | TAG_BOOLEAN
 |}

==== Update Recipes ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="4"| 0x73
 | rowspan="4"| Play
 | rowspan="4"| Client
 | colspan="2"| Num Recipes
 | colspan="2"| VarInt
 | Number of elements in the following array.
 |-
 | rowspan="3"| Recipe
 | Type
 | rowspan="3"| Array
 | Identifier
 | The recipe type, see below.
 |-
 | Recipe ID
 | Identifier
 |
 |-
 | Data
 | Varies
 | Additional data for the recipe.
 |}

Recipe types:

{| class="wikitable"
 ! Type
 ! Description
 ! Data
 |-
 | <code>minecraft:crafting_shapeless</code>
 | Shapeless crafting recipe. All items in the ingredient list must be present, but in any order/slot.
 | As follows:
   {| class="wikitable"
    ! Name
    ! Type
    ! Description
    |-
    | Group
    | String (32767)
    | Used to group similar recipes together in the recipe book. Tag is present in recipe JSON.
    |-
    |Category
    |VarInt Enum
    |Building = 0, Redstone = 1, Equipment = 2, Misc = 3
    |-
    | Ingredient count
    | VarInt
    | Number of elements in the following array.
    |-
    | Ingredients
    | Array of Ingredient.
    |
    |-
    | Result
    | [[Slot]]
    |
    |}
 |-
 | <code>minecraft:crafting_shaped</code>
 | Shaped crafting recipe. All items must be present in the same pattern (which may be flipped horizontally or translated).
 | As follows:
   {| class="wikitable"
    ! Name
    ! Type
    ! Description
    |-
    | Group
    | String (32767)
    | Used to group similar recipes together in the recipe book. Tag is present in recipe JSON.
    |-
    |Category
    |VarInt Enum
    |Building = 0, Redstone = 1, Equipment = 2, Misc = 3
    |-
    | Width
    | VarInt
    |
    |-
    | Height
    | VarInt
    |
    |-
    | Ingredients
    | Array of Ingredient
    | Length is <code>width * height</code>. Indexed by <code>x + (y * width)</code>.
    |-
    | Result
    | [[Slot]]
    |-
    | Show notification
    | Boolean
    | Show a toast when the recipe is [[Protocol#Update_Recipe_Book|added]].
    |}
 |-
 | <code>minecraft:crafting_special_armordye</code>
 | Recipe for dying leather armor
 | rowspan="14" | As follows:
   {| class="wikitable"
    ! Name
    ! Type
    ! Description
    |-
    |Category
    |VarInt Enum
    |Building = 0, Redstone = 1, Equipment = 2, Misc = 3
    |}
 |-
 | <code>minecraft:crafting_special_bookcloning</code>
 | Recipe for copying contents of written books
 |-
 | <code>minecraft:crafting_special_mapcloning</code>
 | Recipe for copying maps
 |-
 | <code>minecraft:crafting_special_mapextending</code>
 | Recipe for adding paper to maps
 |-
 | <code>minecraft:crafting_special_firework_rocket</code>
 | Recipe for making firework rockets
 |-
 | <code>minecraft:crafting_special_firework_star</code>
 | Recipe for making firework stars
 |-
 | <code>minecraft:crafting_special_firework_star_fade</code>
 | Recipe for making firework stars fade between multiple colors
 |-
 | <code>minecraft:crafting_special_repairitem</code>
 | Recipe for repairing items via crafting
 |-
 | <code>minecraft:crafting_special_tippedarrow</code>
 | Recipe for crafting tipped arrows
 |-
 | <code>minecraft:crafting_special_bannerduplicate</code>
 | Recipe for copying banner patterns
 |-
 | <code>minecraft:crafting_special_shielddecoration</code>
 | Recipe for applying a banner's pattern to a shield
 |-
 | <code>minecraft:crafting_special_shulkerboxcoloring</code>
 | Recipe for recoloring a shulker box
 |-
 | <code>minecraft:crafting_special_suspiciousstew</code>
 | Recipe for crafting suspicious stews
 |-
 | <code>minecraft:crafting_decorated_pot</code>
 | Recipe for crafting decorated pots
 |-
 | <code>minecraft:smelting</code>
 | Smelting recipe
 | rowspan="4"| As follows:
   {| class="wikitable"
    ! Name
    ! Type
    ! Description
    |-
    | Group
    | String (32767)
    | Used to group similar recipes together in the recipe book.
    |-
    |Category
    |VarInt Enum
    |Food = 0, Blocks = 1, Misc = 2
    |-
    | Ingredient
    | Ingredient
    |
    |-
    | Result
    | [[Slot]]
    |
    |-
    | Experience
    | Float
    |
    |-
    | Cooking time
    | VarInt
    |
    |}
 |-
 | <code>minecraft:blasting</code>
 | Blast furnace recipe
 |-
 | <code>minecraft:smoking</code>
 | Smoker recipe
 |-
 | <code>minecraft:campfire_cooking</code>
 | Campfire recipe
 |-
 | <code>minecraft:stonecutting</code>
 | Stonecutter recipe
 | As follows:
   {| class="wikitable"
    ! Name
    ! Type
    ! Description
    |-
    | Group
    | String (32767)
    | Used to group similar recipes together in the recipe book.  Tag is present in recipe JSON.
    |-
    | Ingredient
    | Ingredient
    |
    |-
    | Result
    | [[Slot]]
    |
    |}
 |-
 | <code>minecraft:smithing_transform</code>
 | Recipe for smithing netherite gear
 | As follows:
   {| class="wikitable"
    ! Name
    ! Type
    ! Description
    |-
    | Template
    | Ingredient
    | The smithing template.
    |-
    | Base
    | Ingredient
    | The base item.
    |-
    | Addition
    | Ingredient
    | The additional ingredient.
    |-
    | Result
    | [[Slot]]
    |
    |}
 |-
 | <code>minecraft:smithing_trim</code>
 | Recipe for applying armor trims
 | As follows:
   {| class="wikitable"
    ! Name
    ! Type
    ! Description
    |-
    | Template
    | Ingredient
    | The smithing template.
    |-
    | Base
    | Ingredient
    | The base item.
    |-
    | Addition
    | Ingredient
    | The additional ingredient.
    |}
 |}

Ingredient is defined as:

{| class="wikitable"
 ! Name
 ! Type
 ! Description
 |-
 | Count
 | VarInt
 | Number of elements in the following array.
 |-
 | Items
 | Array of [[Slot]]
 | Any item in this array may be used for the recipe.  The count of each item should be 1.
 |}

==== Update Tags (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="3"| 0x74
 | rowspan="3"| Play
 | rowspan="3"| Client
 | colspan="2"| Length of the array
 | colspan="2"| VarInt
 |
 |-
 | rowspan="2"| Array of tags
 | Registry
 | rowspan="2"| Array
 | Identifier
 | Registry identifier (Vanilla expects tags for the registries <code>minecraft:block</code>, <code>minecraft:item</code>, <code>minecraft:fluid</code>, <code>minecraft:entity_type</code>, and <code>minecraft:game_event</code>)
 |-
 | Array of Tag
 | (See below)
 |
 |}

Tag arrays look like:

{| class="wikitable"
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | colspan="2"| Length
 | colspan="2"| VarInt
 | Number of elements in the following array
 |-
 | rowspan="3"| Tags
 | Tag name
 | rowspan="3"| Array
 | Identifier
 |
 |-
 | Count
 | VarInt
 | Number of elements in the following array
 |-
 | Entries
 | Array of VarInt
 | Numeric IDs of the given type (block, item, etc.). This list replaces the previous list of IDs for the given tag. If some preexisting tags are left unmentioned, a warning is printed.
 |}

See {{Minecraft Wiki|Tag}} on the Minecraft Wiki for more information, including a list of vanilla tags.

=== Serverbound ===

==== Confirm Teleportation ====

Sent by client as confirmation of [[#Synchronize Player Position|Synchronize Player Position]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x00
 | Play
 | Server
 | Teleport ID
 | VarInt
 | The ID given by the [[#Synchronize Player Position|Synchronize Player Position]] packet.
 |}

==== Query Block Entity Tag ====

Used when <kbd>F3</kbd>+<kbd>I</kbd> is pressed while looking at a block.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x01
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Transaction ID
 | VarInt
 | An incremental ID so that the client can verify that the response matches.
 |-
 | Location
 | Position
 | The location of the block to check.
 |}

==== Change Difficulty ====

Must have at least op level 2 to use.  Appears to only be used on singleplayer; the difficulty buttons are still disabled in multiplayer.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x02
 | Play
 | Server
 | New difficulty
 | Byte
 | 0: peaceful, 1: easy, 2: normal, 3: hard .
 |}

==== Acknowledge Message ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x03
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Message Count
 | VarInt
 | 
 |}

==== Chat Command ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="8"| 0x04
 | rowspan="8"| Play
 | rowspan="8"| Server
 | colspan="2"| Command
 | colspan="2"| String (256)
 | colspan="2"| The command typed by the client.
 |-
 | colspan="2"| Timestamp
 | colspan="2"| Long
 | colspan="2"| The timestamp that the command was executed.
 |-
 | colspan="2"| Salt
 | colspan="2"| Long
 | colspan="2"| The salt for the following argument signatures.
 |-
 | colspan="2"| Array length
 | colspan="2"| VarInt
 | colspan="2"| Number of entries in the following array. The maximum length in Notchian server is 8.
 |-
 | rowspan="2"| Array of argument signatures
 | Argument name
 | rowspan="2"| Array (8)
 | String (16)
 | The name of the argument that is signed by the following signature.
 |-
 | Signature
 | Byte Array (256)
 | The signature that verifies the argument. Always 256 bytes and is not length-prefixed.
 |-
 | colspan="2"| Message Count
 | colspan="2"| VarInt
 | colspan="2"|
 |-
 | colspan="2"| Acknowledged
 | colspan="2"| Fixed BitSet (20)
 | colspan="2"|
 |}

==== Chat Message ====

Used to send a chat message to the server.  The message may not be longer than 256 characters or else the server will kick the client.

The server will broadcast a [[#Player Chat Message|Player Chat Message]] packet with Chat Type <code>minecraft:chat</code> to all players that haven't disabled chat (including the player that sent the message). See [[Chat#Processing chat]] for more information.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="7"| 0x05
 | rowspan="7"| Play
 | rowspan="7"| Server
 | Message
 | String (256)
 |
 |-
 | Timestamp
 | Long
 |
 |-
 | Salt
 | Long
 | The salt used to verify the signature hash.
 |-
 | Has Signature
 | Boolean
 | Whether the next field is present.
 |-
 | Signature
 | Optional Byte Array (256)
 | The signature used to verify the chat message's authentication. When present, always 256 bytes and not length-prefixed.
 |-
 | Message Count
 | VarInt
 |
 |-
 | Acknowledged
 | Fixed BitSet (20)
 | 
 |}

==== Player Session ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="6"| 0x06
 | rowspan="6"| Play
 | rowspan="6"| Server
 | colspan="2"| Session Id
 | UUID
 |
 |-
 | rowspan="5"| Public Key
 | Expires At
 | Long
 | The time the play session key expires in [https://en.wikipedia.org/wiki/Unix_time epoch] milliseconds.
 |-
 | Public Key Length
 | VarInt
 | Length of the proceeding public key. Maximum length in Notchian server is 512 bytes.
 |-
 | Public Key
 | Byte Array (512)
 | A byte array of an X.509-encoded public key.
 |-
 | Key Signature Length
 | VarInt
 | Length of the proceeding key signature array. Maximum length in Notchian server is 4096 bytes.
 |-
 | Key Signature
 | Byte Array (4096)
 | The signature consists of the player UUID, the key expiration timestamp, and the public key data. These values are hashed using [https://en.wikipedia.org/wiki/SHA-1 SHA-1] and signed using Mojang's private [https://en.wikipedia.org/wiki/RSA_(cryptosystem) RSA] key.
 |}

==== Chunk Batch Received ====

Notifies the server that the chunk batch has been received by the client. The server uses the value sent in this packet to adjust the number of chunks to be sent in a batch.

The Notchian server will stop sending further chunk data until the client acknowledges the sent chunk batch. After the first acknowledgement, the server adjusts this number to allow up to 10 unacknowledged batches.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x07
 | rowspan="1"| Play
 | rowspan="1"| Server
 | Chunks per tick
 | Float
 | Desired chunks per tick.
 |}

==== Client Status ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x08
 | Play
 | Server
 | Action ID
 | VarInt Enum
 | See below
 |}

''Action ID'' values:

{| class="wikitable"
 |-
 ! Action ID
 ! Action
 ! Notes
 |-
 | 0
 | Perform respawn
 | Sent when the client is ready to complete login and when the client is ready to respawn after death.
 |-
 | 1
 | Request stats
 | Sent when the client opens the Statistics menu.
 |}

==== Client Information (play) ====

Sent when the player connects, or when settings are changed.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| 0x09
 | rowspan="8"| Play
 | rowspan="8"| Server
 | Locale
 | String (16)
 | e.g. <code>en_GB</code>.
 |-
 | View Distance
 | Byte
 | Client-side render distance, in chunks.
 |-
 | Chat Mode
 | VarInt Enum
 | 0: enabled, 1: commands only, 2: hidden.  See [[Chat#Client chat mode]] for more information.
 |-
 | Chat Colors
 | Boolean
 | “Colors” multiplayer setting. Can the chat be colored?
 |-
 | Displayed Skin Parts
 | Unsigned Byte
 | Bit mask, see below.
 |-
 | Main Hand
 | VarInt Enum
 | 0: Left, 1: Right.
 |-
 | Enable text filtering
 | Boolean
 | Enables filtering of text on signs and written book titles. Currently always false (i.e. the filtering is disabled)
 |-
 | Allow server listings
 | Boolean
 | Servers usually list online players, this option should let you not show up in that list.
 |}

''Displayed Skin Parts'' flags:

* Bit 0 (0x01): Cape enabled
* Bit 1 (0x02): Jacket enabled
* Bit 2 (0x04): Left Sleeve enabled
* Bit 3 (0x08): Right Sleeve enabled
* Bit 4 (0x10): Left Pants Leg enabled
* Bit 5 (0x20): Right Pants Leg enabled
* Bit 6 (0x40): Hat enabled

The most significant bit (bit 7, 0x80) appears to be unused.

==== Command Suggestions Request ====

Sent when the client needs to tab-complete a <code>minecraft:ask_server</code> suggestion type.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x0A
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Transaction Id
 | VarInt
 | The id of the transaction that the server will send back to the client in the response of this packet. Client generates this and increments it each time it sends another tab completion that doesn't get a response.
 |-
 | Text
 | String (32500)
 | All text behind the cursor without the <code>/</code> (e.g. to the left of the cursor in left-to-right languages like English).
 |}

==== Acknowledge Configuration ====

Sent by the client upon receiving a [[#Start Configuration|Start Configuration]] packet from the server.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="1"| 0x0B
 | rowspan="1"| Play
 | rowspan="1"| Server
 | colspan="3"| ''no fields''
 |}

This packet switches the connection state to [[#Configuration|configuration]].

==== Click Container Button ====

Used when clicking on window buttons.  Until 1.14, this was only used by enchantment tables.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x0C
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Window ID
 | Byte
 | The ID of the window sent by [[#Open Screen|Open Screen]].
 |-
 | Button ID
 | Byte
 | Meaning depends on window type; see below.
 |}

{| class="wikitable"
 ! Window type
 ! ID
 ! Meaning
 |-
 | rowspan="3"| Enchantment Table
 | 0 || Topmost enchantment.
 |-
 | 1 || Middle enchantment.
 |-
 | 2 || Bottom enchantment.
 |-
 | rowspan="4"| Lectern
 | 1 || Previous page (which does give a redstone output).
 |-
 | 2 || Next page.
 |-
 | 3 || Take Book.
 |-
 | 100+page || Opened page number - 100 + number.
 |-
 | Stonecutter
 | colspan="2"| Recipe button number - 4*row + col.  Depends on the item.
 |-
 | Loom
 | colspan="2"| Recipe button number - 4*row + col.  Depends on the item.
 |}

==== Click Container ====

This packet is sent by the client when the player clicks on a slot in a window.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! colspan="2"| Field Name
 ! colspan="2"| Field Type
 ! Notes
 |-
 | rowspan="9"| 0x0D
 | rowspan="9"| Play
 | rowspan="9"| Server
 | colspan="2"| Window ID
 | colspan="2"| Unsigned Byte
 | colspan="2"| The ID of the window which was clicked. 0 for player inventory. The server ignores any packets targeting a Window ID other than the current one, including ignoring 0 when any other window is open.
 |-
 | colspan="2"| State ID
 | colspan="2"| VarInt
 | colspan="2"| The last received State ID from either a [[#Set Container Slot|Set Container Slot]] or a [[#Set Container Content|Set Container Content]] packet.
 |-
 | colspan="2"| Slot
 | colspan="2"| Short
 | colspan="2"| The clicked slot number, see below.
 |-
 | colspan="2"| Button
 | colspan="2"| Byte
 | colspan="2"| The button used in the click, see below.
 |-
 | colspan="2"| Mode
 | colspan="2"| VarInt Enum
 | colspan="2"| Inventory operation mode, see below.
 |-
 | colspan="2"| Length of the array
 | colspan="2"| VarInt
 | colspan="2"| Maximum value for Notchian server is 128 slots.
 |-
 | rowspan="2"| Array of changed slots
 | Slot number
 | rowspan="2"| Array (128)
 | Short
 |
 |-
 | Slot data
 | Slot
 | New data for this slot, in the client's opinion; see below.
 |-
 | colspan="2"| Carried item
 | colspan="2"| [[Slot Data|Slot]]
 | colspan="2"| Item carried by the cursor. Has to be empty (item ID = -1) for drop mode, otherwise nothing will happen.
 |}

See [[Inventory]] for further information about how slots are indexed.

After performing the action, the server compares the results to the slot change information included in the packet, as applied on top of the server's view of the container's state prior to the action. For any slots that do not match, it sends [[#Set Container Slot|Set Container Slot]] packets containing the correct results. If State ID does not match the last ID sent by the server, it will instead send a full [[#Set Container Content|Set Container Content]] to resynchronize the client.

When right-clicking on a stack of items, half the stack will be picked up and half left in the slot. If the stack is an odd number, the half left in the slot will be smaller of the amounts.

The distinct type of click performed by the client is determined by the combination of the Mode and Button fields.

{| class="wikitable"
 ! Mode
 ! Button
 ! Slot
 ! Trigger
 |-
 ! rowspan="4"| 0
 | 0
 | Normal
 | Left mouse click
 |-
 | 1
 | Normal
 | Right mouse click
 |-
 | 0
 | -999
 | Left click outside inventory (drop cursor stack)
 |-
 | 1
 | -999
 | Right click outside inventory (drop cursor single item)
 |-
 ! rowspan="2"| 1
 | 0
 | Normal
 | Shift + left mouse click
 |-
 | 1
 | Normal
 | Shift + right mouse click ''(identical behavior)''
 |-
 ! rowspan="7"| 2
 | 0
 | Normal
 | Number key 1
 |-
 | 1
 | Normal
 | Number key 2
 |-
 | 2
 | Normal
 | Number key 3
 |-
 | ⋮
 | ⋮
 | ⋮
 |-
 | 8
 | Normal
 | Number key 9
 |-
 | ⋮
 | ⋮
 | Button is used as the slot index (impossible in vanilla clients)
 |-
 | 40
 | Normal
 | Offhand swap key F
 |-
 ! 3
 | 2
 | Normal
 | Middle click, only defined for creative players in non-player inventories.
 |-
 ! rowspan="2"| 4
 | 0
 | Normal*
 | Drop key (Q) (* Clicked item is always empty)
 |-
 | 1
 | Normal*
 | Control + Drop key (Q) (* Clicked item is always empty)
 |-
 ! rowspan="9"| 5
 | 0
 | -999
 | Starting left mouse drag
 |-
 | 4
 | -999
 | Starting right mouse drag
 |-
 | 8
 | -999
 | Starting middle mouse drag, only defined for creative players in non-player inventories.
 |-
 | 1
 | Normal
 | Add slot for left-mouse drag
 |-
 | 5
 | Normal
 | Add slot for right-mouse drag
 |-
 | 9
 | Normal
 | Add slot for middle-mouse drag, only defined for creative players in non-player inventories.
 |-
 | 2
 | -999
 | Ending left mouse drag
 |-
 | 6
 | -999
 | Ending right mouse drag
 |-
 | 10
 | -999
 | Ending middle mouse drag, only defined for creative players in non-player inventories.
 |-
 ! rowspan="2"| 6
 | 0
 | Normal
 | Double click
 |-
 | 1
 | Normal
 | Pickup all but check items in reverse order (impossible in vanilla clients)
 |}

Starting from version 1.5, “painting mode” is available for use in inventory windows. It is done by picking up stack of something (more than 1 item), then holding mouse button (left, right or middle) and dragging held stack over empty (or same type in case of right button) slots. In that case client sends the following to server after mouse button release (omitting first pickup packet which is sent as usual):

# packet with mode 5, slot -999, button (0 for left | 4 for right);
# packet for every slot painted on, mode is still 5, button (1 | 5);
# packet with mode 5, slot -999, button (2 | 6);

If any of the painting packets other than the “progress” ones are sent out of order (for example, a start, some slots, then another start; or a left-click in the middle) the painting status will be reset.

==== Close Container ====

This packet is sent by the client when closing a window.

Notchian clients send a Close Window packet with Window ID 0 to close their inventory even though there is never an [[#Open Screen|Open Screen]] packet for the inventory.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x0E
 | Play
 | Server
 | Window ID
 | Unsigned Byte
 | This is the ID of the window that was closed. 0 for player inventory.
 |}

==== Change Container Slot State ====

This packet is sent by the client when toggling the state of a Crafter.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x0F
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Slot ID
 | VarInt
 | This is the ID of the slot that was changed.
 |-
 | Window ID
 | VarInt
 | This is the ID of the window that was changed.
 |-
 | State
 | Boolean
 | The new state of the slot. True for enabled, false for disabled.
 |}

==== Serverbound Plugin Message (play) ====

{{Main|Plugin channels}}

Mods and plugins can use this to send their data. Minecraft itself uses some [[plugin channel]]s. These internal channels are in the <code>minecraft</code> namespace.

More documentation on this: [https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/ https://dinnerbone.com/blog/2012/01/13/minecraft-plugin-channels-messaging/]

Note that the length of Data is known only from the packet length, since the packet has no length field of any kind.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x10
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Channel
 | Identifier
 | Name of the [[plugin channel]] used to send the data.
 |-
 | Data
 | Byte Array (32767)
 | Any data, depending on the channel. <code>minecraft:</code> channels are documented [[plugin channel|here]]. The length of this array must be inferred from the packet length.
 |}

In Notchian server, the maximum data length is 32767 bytes.

==== Edit Book ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x11
 | rowspan="5"| Play
 | rowspan="5"| Server
 | Slot
 | VarInt
 | The hotbar slot where the written book is located
 |-
 | Count
 | VarInt
 | Number of elements in the following array. Maximum array size is 200.
 |-
 | Entries
 | Array (200) of Strings (8192)
 | Text from each page. Maximum string length is 8192 chars.
 |-
 | Has title
 | Boolean
 | If true, the next field is present. true if book is being signed, false if book is being edited.
 |-
 | Title
 | Optional String (128)
 | Title of book.
 |}

==== Query Entity Tag ====

Used when <kbd>F3</kbd>+<kbd>I</kbd> is pressed while looking at an entity.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x12
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Transaction ID
 | VarInt
 | An incremental ID so that the client can verify that the response matches.
 |-
 | Entity ID
 | VarInt
 | The ID of the entity to query.
 |}

==== Interact ====

This packet is sent from the client to the server when the client attacks or right-clicks another entity (a player, minecart, etc).

A Notchian server only accepts this packet if the entity being attacked/used is visible without obstruction and within a 4-unit radius of the player's position.

The target X, Y, and Z fields represent the difference between the vector location of the cursor at the time of the packet and the entity's position.

Note that middle-click in creative mode is interpreted by the client and sent as a [[#Set Creative Mode Slot|Set Creative Mode Slot]] packet instead.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="7"| 0x13
 | rowspan="7"| Play
 | rowspan="7"| Server
 | Entity ID
 | VarInt
 | The ID of the entity to interact.
 |-
 | Type
 | VarInt Enum
 | 0: interact, 1: attack, 2: interact at.
 |-
 | Target X
 | Optional Float
 | Only if Type is interact at.
 |-
 | Target Y
 | Optional Float
 | Only if Type is interact at.
 |-
 | Target Z
 | Optional Float
 | Only if Type is interact at.
 |-
 | Hand
 | Optional VarInt Enum
 | Only if Type is interact or interact at; 0: main hand, 1: off hand.
 |-
 | Sneaking
 | Boolean
 | If the client is sneaking.
 |}

==== Jigsaw Generate ====

Sent when Generate is pressed on the {{Minecraft Wiki|Jigsaw Block}} interface.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x14
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Location
 | Position
 | Block entity location.
 |-
 | Levels
 | VarInt
 | Value of the levels slider/max depth to generate.
 |-
 | Keep Jigsaws
 | Boolean
 |
 |}

==== Serverbound Keep Alive (play) ====

The server will frequently send out a keep-alive (see [[#Clientbound Keep Alive (play)|Clientbound Keep Alive]]), each containing a random ID. The client must respond with the same packet.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x15
 | Play
 | Server
 | Keep Alive ID
 | Long
 |
 |}

==== Lock Difficulty ====

Must have at least op level 2 to use.  Appears to only be used on singleplayer; the difficulty buttons are still disabled in multiplayer.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x16
 | Play
 | Server
 | Locked
 | Boolean
 |
 |}

==== Set Player Position ====

Updates the player's XYZ position on the server.

Checking for moving too fast is achieved like this:

* Each server tick, the player's current position is stored
* When a player moves, the changes in x, y, and z coordinates are compared with the positions from the previous tick (&Delta;x, &Delta;y, &Delta;z)
* Total movement distance squared is computed as &Delta;x&sup2; + &Delta;y&sup2; + &Delta;z&sup2;
* The expected movement distance squared is computed as velocityX&sup2; + velocityY&sup2; + velocityZ&sup2;
* If the total movement distance squared value minus the expected movement distance squared value is more than 100 (300 if the player is using an elytra), they are moving too fast.

If the player is moving too fast, it will be logged that "<player> moved too quickly! " followed by the change in x, y, and z, and the player will be teleported back to their current (before this packet) serverside position.

Also, if the absolute value of X or the absolute value of Z is a value greater than 3.2×10<sup>7</sup>, or X, Y, or Z are not finite (either positive infinity, negative infinity, or NaN), the client will be kicked for “Invalid move player packet received”.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| 0x17
 | rowspan="4"| Play
 | rowspan="4"| Server
 | X
 | Double
 | Absolute position.
 |-
 | Feet Y
 | Double
 | Absolute feet position, normally Head Y - 1.62.
 |-
 | Z
 | Double
 | Absolute position.
 |-
 | On Ground
 | Boolean
 | True if the client is on the ground, false otherwise.
 |}

==== Set Player Position and Rotation ====

A combination of [[#Set Player Rotation|Move Player Rotation]] and [[#Set Player Position|Move Player Position]].

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="6"| 0x18
 | rowspan="6"| Play
 | rowspan="6"| Server
 | X
 | Double
 | Absolute position.
 |-
 | Feet Y
 | Double
 | Absolute feet position, normally Head Y - 1.62.
 |-
 | Z
 | Double
 | Absolute position.
 |-
 | Yaw
 | Float
 | Absolute rotation on the X Axis, in degrees.
 |-
 | Pitch
 | Float
 | Absolute rotation on the Y Axis, in degrees.
 |-
 | On Ground
 | Boolean
 | True if the client is on the ground, false otherwise.
 |}

==== Set Player Rotation ====
[[File:Minecraft-trig-yaw.png|thumb|The unit circle for yaw]]
[[File:Yaw.png|thumb|The unit circle of yaw, redrawn]]

Updates the direction the player is looking in.

Yaw is measured in degrees, and does not follow classical trigonometry rules. The unit circle of yaw on the XZ-plane starts at (0, 1) and turns counterclockwise, with 90 at (-1, 0), 180 at (0,-1) and 270 at (1, 0). Additionally, yaw is not clamped to between 0 and 360 degrees; any number is valid, including negative numbers and numbers greater than 360.

Pitch is measured in degrees, where 0 is looking straight ahead, -90 is looking straight up, and 90 is looking straight down.

The yaw and pitch of player (in degrees), standing at point (x0, y0, z0) and looking towards point (x, y, z) can be calculated with:

 dx = x-x0
 dy = y-y0
 dz = z-z0
 r = sqrt( dx*dx + dy*dy + dz*dz )
 yaw = -atan2(dx,dz)/PI*180
 if yaw < 0 then
     yaw = 360 + yaw
 pitch = -arcsin(dy/r)/PI*180

You can get a unit vector from a given yaw/pitch via:

 x = -cos(pitch) * sin(yaw)
 y = -sin(pitch)
 z =  cos(pitch) * cos(yaw)

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x19
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Yaw
 | Float
 | Absolute rotation on the X Axis, in degrees.
 |-
 | Pitch
 | Float
 | Absolute rotation on the Y Axis, in degrees.
 |-
 | On Ground
 | Boolean
 | True if the client is on the ground, false otherwise.
 |}

==== Set Player On Ground ====

This packet as well as [[#Set Player Position|Set Player Position]], [[#Set Player Rotation|Set Player Rotation]], and [[#Set Player Position and Rotation|Set Player Position and Rotation]] are called the “serverbound movement packets”. Vanilla clients will send Move Player Position once every 20 ticks even for a stationary player.

This packet is used to indicate whether the player is on ground (walking/swimming), or airborne (jumping/falling).

When dropping from sufficient height, fall damage is applied when this state goes from false to true. The amount of damage applied is based on the point where it last changed from true to false. Note that there are several movement related packets containing this state.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x1A
 | Play
 | Server
 | On Ground
 | Boolean
 | True if the client is on the ground, false otherwise.
 |}

==== Move Vehicle ====

Sent when a player moves in a vehicle. Fields are the same as in [[#Set Player Position and Rotation|Set Player Position and Rotation]]. Note that all fields use absolute positioning and do not allow for relative positioning.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x1B
 | rowspan="5"| Play
 | rowspan="5"| Server
 | X
 | Double
 | Absolute position (X coordinate).
 |-
 | Y
 | Double
 | Absolute position (Y coordinate).
 |-
 | Z
 | Double
 | Absolute position (Z coordinate).
 |-
 | Yaw
 | Float
 | Absolute rotation on the vertical axis, in degrees.
 |-
 | Pitch
 | Float
 | Absolute rotation on the horizontal axis, in degrees.
 |}

==== Paddle Boat ====

Used to ''visually'' update whether boat paddles are turning.  The server will update the [[Entity_metadata#Boat|Boat entity metadata]] to match the values here.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x1C
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Left paddle turning
 | Boolean
 |
 |-
 | Right paddle turning
 | Boolean
 |
 |}

Right paddle turning is set to true when the left button or forward button is held, left paddle turning is set to true when the right button or forward button is held.

==== Pick Item ====

Used to swap out an empty space on the hotbar with the item in the given inventory slot.  The Notchian client uses this for pick block functionality (middle click) to retrieve items from the inventory.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x1D
 | Play
 | Server
 | Slot to use
 | VarInt
 | See [[Inventory]].
 |}

The server first searches the player's hotbar for an empty slot, starting from the current slot and looping around to the slot before it.  If there are no empty slots, it starts a second search from the current slot and finds the first slot that does not contain an enchanted item.  If there still are no slots that meet that criteria, then the server uses the currently selected slot.

After finding the appropriate slot, the server swaps the items and sends 3 packets:

* [[#Set Container Slot|Set Container Slot]] with window ID set to -2, updating the chosen hotbar slot.
* [[#Set Container Slot|Set Container Slot]] with window ID set to -2, updating the slot where the picked item used to be.
* [[#Set Held Item|Set Held Item]], switching to the newly chosen slot.

==== Ping Request (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x1E
 | Play
 | Server
 | Payload
 | Long
 | May be any number. Notchian clients use a system-dependent time value which is counted in milliseconds.
 |}

==== Place Recipe ====

This packet is sent when a player clicks a recipe in the crafting book that is craftable (white border).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x1F
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Window ID
 | Byte
 |
 |-
 | Recipe
 | Identifier
 | A recipe ID.
 |-
 | Make all
 | Boolean
 | Affects the amount of items processed; true if shift is down when clicked.
 |}

==== Player Abilities ====

The vanilla client sends this packet when the player starts/stops flying with the Flags parameter changed accordingly.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x20
 | Play
 | Server
 | Flags
 | Byte
 | Bit mask. 0x02: is flying.
 |}

==== Player Action ====

Sent when the player mines a block. A Notchian server only accepts digging packets with coordinates within a 6-unit radius between the center of the block and 1.5 units from the player's feet (''not'' their eyes).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="4"| 0x21
 | rowspan="4"| Play
 | rowspan="4"| Server
 | Status
 | VarInt Enum
 | The action the player is taking against the block (see below).
 |-
 | Location
 | Position
 | Block position.
 |-
 | Face
 | Byte Enum
 | The face being hit (see below).
 |-
 | Sequence
 | VarInt
 | Block change sequence number (see [[#Acknowledge Block Change]]).
 |}

Status can be one of seven values:

{| class="wikitable"
 ! Value
 ! Meaning
 ! Notes
 |-
 | 0
 | Started digging
 | Sent when the player starts digging a block. If the block was instamined or the player is in creative mode, the client will ''not'' send Status = Finished digging, and will assume the server completed the destruction. To detect this, it is necessary to {{Minecraft Wiki|Breaking#Speed|calculate the block destruction speed}} server-side.
 |-
 | 1
 | Cancelled digging
 | Sent when the player lets go of the Mine Block key (default: left click).
 |-
 | 2
 | Finished digging
 | Sent when the client thinks it is finished.
 |-
 | 3
 | Drop item stack
 | Triggered by using the Drop Item key (default: Q) with the modifier to drop the entire selected stack (default: Control or Command, depending on OS). Location is always set to 0/0/0, Face is always set to -Y. Sequence is always set to 0.
 |-
 | 4
 | Drop item
 | Triggered by using the Drop Item key (default: Q). Location is always set to 0/0/0, Face is always set to -Y. Sequence is always set to 0.
 |-
 | 5
 | Shoot arrow / finish eating
 | Indicates that the currently held item should have its state updated such as eating food, pulling back bows, using buckets, etc. Location is always set to 0/0/0, Face is always set to -Y. Sequence is always set to 0.
 |-
 | 6
 | Swap item in hand
 | Used to swap or assign an item to the second hand. Location is always set to 0/0/0, Face is always set to -Y. Sequence is always set to 0.
 |}

The Face field can be one of the following values, representing the face being hit:

{| class="wikitable"
 |-
 ! Value
 ! Offset
 ! Face
 |-
 | 0
 | -Y
 | Bottom
 |-
 | 1
 | +Y
 | Top
 |-
 | 2
 | -Z
 | North
 |-
 | 3
 | +Z
 | South
 |-
 | 4
 | -X
 | West
 |-
 | 5
 | +X
 | East
 |}

==== Player Command ====

Sent by the client to indicate that it has performed certain actions: sneaking (crouching), sprinting, exiting a bed, jumping with a horse, and opening a horse's inventory while riding it.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x22
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Entity ID
 | VarInt
 | Player ID
 |-
 | Action ID
 | VarInt Enum
 | The ID of the action, see below.
 |-
 | Jump Boost
 | VarInt
 | Only used by the “start jump with horse” action, in which case it ranges from 0 to 100. In all other cases it is 0.
 |}

Action ID can be one of the following values:

{| class="wikitable"
 ! ID
 ! Action
 |-
 | 0
 | Start sneaking
 |-
 | 1
 | Stop sneaking
 |-
 | 2
 | Leave bed
 |-
 | 3
 | Start sprinting
 |-
 | 4
 | Stop sprinting
 |-
 | 5
 | Start jump with horse
 |-
 | 6
 | Stop jump with horse
 |-
 | 7
 | Open vehicle inventory
 |-
 | 8
 | Start flying with elytra
 |}

Leave bed is only sent when the “Leave Bed” button is clicked on the sleep GUI, not when waking up in the morning.

Open vehicle inventory is only sent when pressing the inventory key (default: E) while on a horse or chest boat — all other methods of opening such an inventory (involving right-clicking or shift-right-clicking it) do not use this packet.

==== Player Input ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x23
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Sideways
 | Float
 | Positive to the left of the player.
 |-
 | Forward
 | Float
 | Positive forward.
 |-
 | Flags
 | Unsigned Byte
 | Bit mask. 0x1: jump, 0x2: unmount.
 |}

Also known as 'Input' packet.

==== Pong (play) ====

Response to the clientbound packet ([[#Ping (play)|Ping]]) with the same id.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x24
 | Play
 | Server
 | ID
 | Int
 | id is the same as the ping packet
 |}

==== Change Recipe Book Settings ====

Replaces Recipe Book Data, type 1.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x25
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Book ID
 | VarInt Enum
 | 0: crafting, 1: furnace, 2: blast furnace, 3: smoker.
 |-
 | Book Open
 | Boolean
 |
 |-
 | Filter Active
 | Boolean
 |
 |}

==== Set Seen Recipe ====

Sent when recipe is first seen in recipe book. Replaces Recipe Book Data, type 0.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x26
 | Play
 | Server
 | Recipe ID
 | Identifier
 |
 |}

==== Rename Item ====

Sent as a player is renaming an item in an anvil (each keypress in the anvil UI sends a new Rename Item packet). If the new name is empty, then the item loses its custom name (this is different from setting the custom name to the normal name of the item). The item name may be no longer than 50 characters long, and if it is longer than that, then the rename is silently ignored.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x27
 | Play
 | Server
 | Item name
 | String (32767)
 | The new name of the item.
 |}

==== Resource Pack Response (play) ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3" | 0x28
 | rowspan="3" | Play
 | rowspan="3" | Server
 |-
 | UUID
 | UUID
 | The unique identifier of the resource pack received in the [[#Add_Resource_Pack_(play)|Add Resource Pack (play)]] request.
 |-
 | Result
 | VarInt Enum
 | Result ID (see below).
 |}

Result can be one of the following values:

{| class="wikitable"
 ! ID
 ! Result
 |-
 | 0
 | Successfully downloaded
 |-
 | 1
 | Declined
 |-
 | 2
 | Failed to download
 |-
 | 3
 | Accepted
 |-
 | 4
 | Invalid URL
 |-
 | 5
 | Failed to reload
 |-
 | 6
 | Discarded
 |}


==== Seen Advancements ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x29
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Action
 | VarInt Enum
 | 0: Opened tab, 1: Closed screen.
 |-
 | Tab ID
 | Optional identifier
 | Only present if action is Opened tab.
 |}

==== Select Trade ====

When a player selects a specific trade offered by a villager NPC.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x2A
 | Play
 | Server
 | Selected slot
 | VarInt
 | The selected slot in the players current (trading) inventory.
 |}

==== Set Beacon Effect ====

Changes the effect of the current beacon.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x2B
 | rowspan="5"| Play
 | rowspan="5"| Server
 |-
 | Has Primary Effect
 | Boolean
 |-
 | Primary Effect
 | VarInt
 | A [https://minecraft.wiki/w/Potion#ID Potion ID].
 |-
 | Has Secondary Effect
 | Boolean
 |
 |-
 | Secondary Effect
 | VarInt
 | A [https://minecraft.wiki/w/Potion#ID Potion ID].
 |}

==== Set Held Item ====

Sent when the player changes the slot selection

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x2C
 | Play
 | Server
 | Slot
 | Short
 | The slot which the player has selected (0–8).
 |}

==== Program Command Block ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="5"| 0x2D
 | rowspan="5"| Play
 | rowspan="5"| Server
 |-
 | Location
 | Position
 |
 |-
 | Command
 | String (32767)
 |
 |-
 | Mode || VarInt Enum || One of SEQUENCE (0), AUTO (1), or REDSTONE (2).
 |-
 | Flags
 | Byte
 | 0x01: Track Output (if false, the output of the previous command will not be stored within the command block); 0x02: Is conditional; 0x04: Automatic.
 |}

==== Program Command Block Minecart ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="3"| 0x2E
 | rowspan="3"| Play
 | rowspan="3"| Server
 | Entity ID
 | VarInt
 |
 |-
 | Command
 | String (32767)
 |
 |-
 | Track Output
 | Boolean
 | If false, the output of the previous command will not be stored within the command block.
 |}

==== Set Creative Mode Slot ====

While the user is in the standard inventory (i.e., not a crafting bench) in Creative mode, the player will send this packet.

Clicking in the creative inventory menu is quite different from non-creative inventory management. Picking up an item with the mouse actually deletes the item from the server, and placing an item into a slot or dropping it out of the inventory actually tells the server to create the item from scratch. (This can be verified by clicking an item that you don't mind deleting, then severing the connection to the server; the item will be nowhere to be found when you log back in.) As a result of this implementation strategy, the "Destroy Item" slot is just a client-side implementation detail that means "I don't intend to recreate this item.". Additionally, the long listings of items (by category, etc.) are a client-side interface for choosing which item to create. Picking up an item from such listings sends no packets to the server; only when you put it somewhere does it tell the server to create the item in that location.

This action can be described as "set inventory slot". Picking up an item sets the slot to item ID -1. Placing an item into an inventory slot sets the slot to the specified item. Dropping an item (by clicking outside the window) effectively sets slot -1 to the specified item, which causes the server to spawn the item entity, etc.. All other inventory slots are numbered the same as the non-creative inventory (including slots for the 2x2 crafting menu, even though they aren't visible in the vanilla client).

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x2F
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Slot
 | Short
 | Inventory slot.
 |-
 | Clicked Item
 | [[Slot Data|Slot]]
 |
 |}

==== Program Jigsaw Block ====

Sent when Done is pressed on the {{Minecraft Wiki|Jigsaw Block}} interface.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| 0x30
 | rowspan="8"| Play
 | rowspan="8"| Server
 | Location
 | Position
 | Block entity location
 |-
 | Name
 | Identifier
 |
 |-
 | Target
 | Identifier
 |
 |-
 | Pool
 | Identifier
 |
 |-
 | Final state
 | String (32767)
 | "Turns into" on the GUI, <code>final_state</code> in NBT.
 |-
 | Joint type
 | String (32767)
 | <code>rollable</code> if the attached piece can be rotated, else <code>aligned</code>.
 |-
 | Selection priority
 | VarInt
 |
 |-
 | Placement priority
 | VarInt
 |
 |}

[[Category:Minecraft Modern]]


==== Program Structure Block ====

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="17"| 0x31
 | rowspan="17"| Play
 | rowspan="17"| Server
 |-
 | Location
 | Position
 | Block entity location.
 |-
 | Action
 | VarInt Enum
 | An additional action to perform beyond simply saving the given data; see below.
 |-
 | Mode
 | VarInt Enum
 | One of SAVE (0), LOAD (1), CORNER (2), DATA (3).
 |-
 | Name
 | String (32767)
 |
 |-
 | Offset X || Byte
 | Between -48 and 48.
 |-
 | Offset Y || Byte
 | Between -48 and 48.
 |-
 | Offset Z || Byte
 | Between -48 and 48.
 |-
 | Size X || Byte
 | Between 0 and 48.
 |-
 | Size Y || Byte
 | Between 0 and 48.
 |-
 | Size Z || Byte
 | Between 0 and 48.
 |-
 | Mirror
 | VarInt Enum
 | One of NONE (0), LEFT_RIGHT (1), FRONT_BACK (2).
 |-
 | Rotation
 | VarInt Enum
 | One of NONE (0), CLOCKWISE_90 (1), CLOCKWISE_180 (2), COUNTERCLOCKWISE_90 (3).
 |-
 | Metadata
 | String (128)
 |
 |-
 | Integrity
 | Float
 | Between 0 and 1.
 |-
 |Seed
 |VarLong
 |
 |-
 | Flags
 | Byte
 | 0x01: Ignore entities; 0x02: Show air; 0x04: Show bounding box.
 |}

Possible actions:

* 0 - Update data
* 1 - Save the structure
* 2 - Load the structure
* 3 - Detect size

The Notchian client uses update data to indicate no special action should be taken (i.e. the done button).

==== Update Sign ====

This message is sent from the client to the server when the “Done” button is pushed after placing a sign.

The server only accepts this packet after [[#Open Sign Editor|Open Sign Editor]], otherwise this packet is silently ignored.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="6"| 0x32
 | rowspan="6"| Play
 | rowspan="6"| Server
 | Location
 | Position
 | Block Coordinates.
 |-
 | Is Front Text
 | Boolean
 | Whether the updated text is in front or on the back of the sign
 |-
 | Line 1
 | String (384)
 | First line of text in the sign.
 |-
 | Line 2
 | String (384)
 | Second line of text in the sign.
 |-
 | Line 3
 | String (384)
 | Third line of text in the sign.
 |-
 | Line 4
 | String (384)
 | Fourth line of text in the sign.
 |}

==== Swing Arm ====

Sent when the player's arm swings.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x33
 | Play
 | Server
 | Hand
 | VarInt Enum
 | Hand used for the animation. 0: main hand, 1: off hand.
 |}

==== Teleport To Entity ====

Teleports the player to the given entity.  The player must be in spectator mode.

The Notchian client only uses this to teleport to players, but it appears to accept any type of entity.  The entity does not need to be in the same dimension as the player; if necessary, the player will be respawned in the right world.  If the given entity cannot be found (or isn't loaded), this packet will be ignored.  It will also be ignored if the player attempts to teleport to themselves.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | 0x34
 | Play
 | Server
 | Target Player
 | UUID
 | UUID of the player to teleport to (can also be an entity UUID).
 |}

==== Use Item On ====

{| class="wikitable"
 |-
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="8"| 0x35
 | rowspan="8"| Play
 | rowspan="8"| Server
 | Hand
 | VarInt Enum
 | The hand from which the block is placed; 0: main hand, 1: off hand.
 |-
 | Location
 | Position
 | Block position.
 |-
 | Face
 | VarInt Enum
 | The face on which the block is placed (as documented at [[#Player Action|Player Action]]).
 |-
 | Cursor Position X
 | Float
 | The position of the crosshair on the block, from 0 to 1 increasing from west to east.
 |-
 | Cursor Position Y
 | Float
 | The position of the crosshair on the block, from 0 to 1 increasing from bottom to top.
 |-
 | Cursor Position Z
 | Float
 | The position of the crosshair on the block, from 0 to 1 increasing from north to south.
 |-
 | Inside block
 | Boolean
 | True when the player's head is inside of a block.
 |-
 | Sequence
 | VarInt
 | Block change sequence number (see [[#Acknowledge Block Change]]).
 |}

Upon placing a block, this packet is sent once.

The Cursor Position X/Y/Z fields (also known as in-block coordinates) are calculated using raytracing. The unit corresponds to sixteen pixels in the default resource pack. For example, let's say a slab is being placed against the south face of a full block. The Cursor Position X will be higher if the player was pointing near the right (east) edge of the face, lower if pointing near the left. The Cursor Position Y will be used to determine whether it will appear as a bottom slab (values 0.0–0.5) or as a top slab (values 0.5-1.0). The Cursor Position Z should be 1.0 since the player was looking at the southernmost part of the block.

Inside block is true when a player's head (specifically eyes) are inside of a block's collision. In 1.13 and later versions, collision is rather complicated and individual blocks can have multiple collision boxes. For instance, a ring of vines has a non-colliding hole in the middle. This value is only true when the player is directly in the box. In practice, though, this value is only used by scaffolding to place in front of the player when sneaking inside of it (other blocks will place behind when you intersect with them -- try with glass for instance).

==== Use Item ====

Sent when pressing the Use Item key (default: right click) with an item in hand.

{| class="wikitable"
 ! Packet ID
 ! State
 ! Bound To
 ! Field Name
 ! Field Type
 ! Notes
 |-
 | rowspan="2"| 0x36
 | rowspan="2"| Play
 | rowspan="2"| Server
 | Hand
 | VarInt Enum
 | Hand used for the animation. 0: main hand, 1: off hand.
 |-
 | Sequence
 | VarInt
 | Block change sequence number (see [[#Acknowledge Block Change]]).
 |}

[[Category:Protocol Details]]
[[Category:Minecraft Modern]]
