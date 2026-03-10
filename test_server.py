#!/usr/bin/env python3
"""
WebSocket 测试服务器（支持 ShirohaChat 协议）
支持功能：
- send_message + send_message_ack
- heartbeat + heartbeat_ack
- receive_message 广播
- msgId 幂等性
"""

import asyncio
import json
import websockets
from datetime import datetime, timezone

# 存储所有连接的客户端
connected_clients = set()

# msgId 幂等性缓存（msgId -> serverId）
processed_messages = {}


def now_iso():
    """返回 ISO 格式的 UTC 时间戳"""
    return datetime.now(timezone.utc).isoformat()


async def handle_client(websocket):
    """处理客户端连接"""
    connected_clients.add(websocket)
    print(f"[{datetime.now().strftime('%H:%M:%S')}] 新客户端连接，当前连接数: {len(connected_clients)}")

    try:
        async for raw_message in websocket:
            print(f"[{datetime.now().strftime('%H:%M:%S')}] 收到: {raw_message}")

            # 解析 JSON
            try:
                packet = json.loads(raw_message)
            except json.JSONDecodeError:
                print(f"[{datetime.now().strftime('%H:%M:%S')}] 无效 JSON，忽略")
                continue

            cmd = packet.get("cmd")
            msg_id = packet.get("msgId")

            # 处理 send_message 命令
            if cmd == "send_message":
                # 幂等性检查
                if msg_id in processed_messages:
                    server_id = processed_messages[msg_id]
                    print(f"[{datetime.now().strftime('%H:%M:%S')}] 重复消息 {msg_id}，返回缓存 ACK")
                else:
                    server_id = f"server_{len(processed_messages) + 1}"
                    processed_messages[msg_id] = server_id

                # 发送 ACK
                ack = {
                    "cmd": "send_message_ack",
                    "msgId": msg_id,
                    "code": 200,
                    "timestamp": now_iso(),
                    "payload": {"serverId": server_id}
                }
                await websocket.send(json.dumps(ack))
                print(f"[{datetime.now().strftime('%H:%M:%S')}] 发送 ACK: {msg_id}")

                # 广播给其他客户端
                payload = packet.get("payload", {})
                broadcast = {
                    "cmd": "receive_message",
                    "msgId": server_id,
                    "timestamp": now_iso(),
                    "payload": {
                        "senderUserId": payload.get("senderUserId"),
                        "content": payload.get("content")
                    }
                }
                broadcast_msg = json.dumps(broadcast)

                disconnected = set()
                for client in connected_clients:
                    if client != websocket:
                        try:
                            await client.send(broadcast_msg)
                            print(f"[{datetime.now().strftime('%H:%M:%S')}] 广播消息给其他客户端")
                        except websockets.exceptions.ConnectionClosed:
                            disconnected.add(client)

                connected_clients.difference_update(disconnected)

            # 处理 heartbeat 命令
            elif cmd == "heartbeat":
                ack = {
                    "cmd": "heartbeat_ack",
                    "msgId": msg_id,
                    "timestamp": now_iso()
                }
                await websocket.send(json.dumps(ack))
                print(f"[{datetime.now().strftime('%H:%M:%S')}] 发送心跳 ACK")

            else:
                print(f"[{datetime.now().strftime('%H:%M:%S')}] 未知命令: {cmd}")

    except websockets.exceptions.ConnectionClosed:
        print(f"[{datetime.now().strftime('%H:%M:%S')}] 客户端断开连接")
    finally:
        connected_clients.discard(websocket)
        print(f"[{datetime.now().strftime('%H:%M:%S')}] 当前连接数: {len(connected_clients)}")


async def main():
    """启动服务器"""
    server = await websockets.serve(handle_client, "localhost", 8080)
    print("=" * 60)
    print("WebSocket 测试服务器已启动（ShirohaChat 协议）")
    print("监听地址: ws://localhost:8080")
    print("支持命令: send_message, heartbeat")
    print("按 Ctrl+C 停止服务器")
    print("=" * 60)
    await server.wait_closed()


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n服务器已停止")
