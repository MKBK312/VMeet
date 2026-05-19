# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

**Client (Windows 11, Qt 5.15.2 MinGW 8.1.0 64-bit):**
```bash
# Recommended: build from Qt Creator (avoids MinGW version mismatch)
# Qt Creator: E:\Qt\Tools\QtCreator\bin\qtcreator.exe

# Command line (must force correct MinGW 8.1.0 — system PATH may resolve to incompatible 15.1.0):
E:\Qt\5.15.2\mingw81_64\bin\qmake.exe IMClient.pro
mingw32-make -j4 CC="E:/Qt/Tools/mingw810_64/bin/gcc" CXX="E:/Qt/Tools/mingw810_64/bin/g++"
```
- Output: `release/IMClient.exe` or `debug/IMClient.exe`
- Requires `-lWs2_32` (Winsock2)

**Server (Linux VM, g++):**
```bash
# SSH to VM first (mkbk@192.168.134.128, password 21SavageAI)
cd /home/mkbk/colin/im_server_linux/
make -j4
# Server binary: /home/mkbk/colin/im_server_linux/bin/im_server
# Stop running server: fuser -k 6789/tcp
# Start server: nohup ./bin/im_server > /tmp/im_server.log 2>&1 &
```

## Architecture

**3-layer design:**
```
UI (LoginDialog, friendList, ChatDialog, room/*)
  ← signals/slots →
CKernel (protocol dispatch, business logic)
  ← sig_dealData →
TcpClientMediator → TCPClient (raw Winsock2 socket)
```

**Protocol dispatch:** Function pointer table `PFUN m_protocol[DEF_PROTOCOL_COUNT]`. Index = `packtype - DEF_PROTOCOL_BASE(1000)`. All handlers live in `ckernel.cpp`.

**Wire format:** `[4-byte length][N-byte struct]` over TCP. Struct first field is always `packageType packtype`.

**Encoding:** Qt uses UTF-8 (QString), network uses GB2312 (char*). `Utf8Togb2312` / `gb2312ToUtf8` in `ckernel.cpp`.

**Naming conventions:**
- Classes: `CKernel`, `LoginDialog`, `friendList` (inconsistent — legacy)
- Signals: `sig_xxx`, slots: `slots_xxx`, auto-connect slots: `on_controlName_eventName()`
- Member variables: `m_` prefix, pointers `m_pXxx`, maps `m_mapXxxToYyy`
- Protocol structs: `PROT_XXX_RQ` (request), `PROT_XXX_RS` (response)
- Protocol constants: `DEF_XXX_YYY`

## Key Files

| File | Purpose |
|------|---------|
| `net/def.h` | All protocol structs, constants, result codes. Must match server exactly. |
| `ckernel.cpp` | Protocol dispatch, all handlers, network send/receive, room logic |
| `ckernel.h` | Handler declarations, member variables, RoomData struct |
| `friendlist.cpp` | Main window with 会议/聊天 tabs, lobby, embedded room view |
| `net/TCPClient.cpp` | Raw Winsock2 TCP with recv thread, length-prefixed framing |
| `Mediator/TcpClientMediator.cpp` | Thin adapter: TCPClient ↔ CKernel via signals |
| `style.qss` | Global stylesheet (loaded from Qt resource) |
| `room/` | RoomCreate dialog (modal). RoomList/RoomChat exist on disk but are no longer instantiated. |

## Server VM

- **SSH:** `ssh mkbk@192.168.134.128` (password `21SavageAI`)
- **MySQL:** root / 041206, database `20240919im`, charset utf8mb4
- **Server source:** `/home/mkbk/colin/im_server_linux/`
- **Server binary:** `/home/mkbk/colin/im_server_linux/bin/im_server`

## Critical Gotchas

1. **MinGW version mismatch:** bash on Windows PATH splitting on `:` breaks Windows drive letters. System MinGW 15.1.0 is ABI-incompatible with Qt's MinGW 8.1.0. Always override CC/CXX when building from command line, or use Qt Creator.
2. **Port was wrong:** Original code used port 67890 (>65535). Fixed to 6789. Both sides must agree.
3. **Protocol struct alignment:** Server and client `def.h` must define structs identically. Any field order, type, or padding difference breaks communication.
4. **`strcpy_s` on GB2312:** GB2312-encoded strings from `Utf8Togb2312` can contain embedded null bytes that truncate with strcpy_s. Use `memcpy` + manual null termination for chat content fields.
5. **Room creator exit:** When the room creator leaves via LEAVE, the room persists (creator can re-join). Room is only destroyed via END_RQ (explicit "End Meeting") or when the creator disconnects entirely (dealOfflineRq).

## Room Feature (protocols 1011-1024)

Protocols: CREATE(1011-1012), JOIN(1013-1014), LEAVE(1015-1016), CHAT(1017-1018), LIST(1019-1020), INFO(1021), END(1022-1023), MEMBER_LIST(1024).

UI: friendList has 2 tabs — "会议" (default, with lobby + embedded room view) and "聊天" (contacts). RoomList/RoomChat classes file-exist but not instantiated.

### Room status (2026-05-16)
- Core room features working: create, join by room number, leave, end meeting, room chat, member list
- Real-time member list updates via server broadcast (ROOM_INFO + MEMBER_LIST on join/leave)
- Client-side 5s polling timer as fallback
- Known unfixed bugs: leave+rejoin loses room state (see `项目进度与计划.md` #1-3)

## Key Documents

| Document | Purpose |
|----------|---------|
| `项目进度与计划.md` | Full project status, known bugs, future roadmap |
| `IMClient开发文档.md` | Original development doc (architecture, flow charts, protocols) |
