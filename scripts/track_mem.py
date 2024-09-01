import asyncio
from signal import SIGINT, SIGTERM
from gdb_ctrl import GDBCtrl
from gdb_mi import StreamRecord, ResultRecord, TerminationRecord, AsyncRecord
import json

from dataclasses import dataclass

from typing import List, Dict, Literal

@dataclass
class Allocation:
    address: int
    size: int
    creator: str
    user_responsible: bool

allocations: Dict[int, Allocation] = {}

async def recv_all(gdb: GDBCtrl) -> List[StreamRecord|ResultRecord|TerminationRecord|AsyncRecord]:
    to_return = []
    resp = await gdb.recv()
    while type(resp) != TerminationRecord:
        to_return.append(resp)
        resp = await gdb.recv()
    return to_return

async def recv_until_token(gdb: GDBCtrl, token: str) -> List[StreamRecord|TerminationRecord|ResultRecord|AsyncRecord]:
    to_return = []
    resp = await gdb.recv()
    while type(resp) == StreamRecord or type(resp) == TerminationRecord or str(resp.token) != token:
        to_return.append(resp)
        resp = await gdb.recv()
    to_return.append(resp)
    return to_return

async def init(gdb: GDBCtrl) -> str:
    token = await gdb.send('-file-exec-and-symbols "/home/ryan/repos/pico-rainviewer/build/pico_rainviewer.elf"')
    resp = await recv_all(gdb)
    #print(resp)
    token = await gdb.send('-target-select extended-remote localhost:3333')
    resp = await recv_all(gdb)
    #print(resp)
    token = await gdb.send('-interpreter-exec console "monitor reset halt"')
    resp = await recv_all(gdb)
    #print(resp)
    token = await gdb.send('-break-insert "/home/ryan/repos/pico-rainviewer/lib/pico-sdk/src/rp2_common/pico_malloc/pico_malloc.c:23"')
    resp = await recv_all(gdb)
    #print(resp)
    # token = await gdb.send('-break-insert "/home/ryan/repos/pico-rainviewer/lib/pico-sdk/src/rp2_common/pico_malloc/pico_malloc.c:65"')
    # resp = await recv_all(gdb)
    # print(resp)
    token = await gdb.send('-break-insert "/home/ryan/repos/pico-rainviewer/lib/pico-sdk/src/rp2_common/pico_malloc/pico_malloc.c:82"')
    resp = await recv_all(gdb)
    #print(resp)
    token = await gdb.send('-exec-continue --all')
    return token

def print_if_token(tokens: List[str], responses: List[StreamRecord|ResultRecord|TerminationRecord|AsyncRecord]) -> bool:
    printed = False
    for res in responses:
        if(type(res) == StreamRecord):
            continue
        if(str(res.token) in tokens):
            print(res)
            tokens.remove(str(res.token))
            printed = True
    return printed

def handle_frames(frames: List[Dict[Literal["frame"], Dict[Literal["level", "addr", "func", "file", "fullname", "line", "arch"], str]]], args: List[Dict[Literal["frame"], Dict[Literal["args"], List[Dict[Literal["name", "value"], str]]]]]):
    arguments = [{arg["name"]: arg["value"] for arg in level_args["frame"]["args"]} for level_args in args]
    
    if frames[0]["frame"]["func"] == "__wrap_free" and arguments[0]["mem"] != "<optimized out>":
        address = int(args[0]["frame"]["args"][0]["value"], 16)
        if address in allocations:
            allocations.pop(address)
    elif arguments[0]["mem"] != "<optimized out>" and ("mem" not in arguments[1] or arguments[1]["mem"] != "<optimized out>"):
        if frames[1]["frame"]["func"] == "__wrap_realloc" and arguments[1]["mem"] != arguments[0]["mem"]:
            address = int(arguments[1]["mem"], 16)
            if address in allocations:
                allocations.pop(address)
        address = int(arguments[0]["mem"], 16)
        size = int(arguments[1]["size"])
        creator = "(system)"
        if frames[-1]["frame"]["func"] == "main" or len(frames) == 30:
            try:
                first_user_frame = next(frame for frame in frames if "fullname" in frame["frame"] and ("pico-rainviewer/src" in frame["frame"]["fullname"] or "pico-web-client/src" in frame["frame"]["fullname"]))
                index = frames.index(first_user_frame)
                print(f"First user frame was index {index}")
                creator = f"{first_user_frame['frame']['func']}:{first_user_frame['frame']['line']}"
            except StopIteration:
                if "fullname" in frames[2]["frame"] and "mbedtls" in frames[2]["frame"]["fullname"]:
                    creator = "(mbedtls)"

        allocations[address] = Allocation(address, size, creator, frames[-1]["frame"]["func"] == "main")

def humanbytes(B):
    """Return the given bytes as a human friendly KB, MB, GB, or TB string."""
    B = float(B)
    KB = float(1024)
    MB = float(KB ** 2) # 1,048,576
    GB = float(KB ** 3) # 1,073,741,824
    TB = float(KB ** 4) # 1,099,511,627,776

    if B < KB:
        return '{0} {1}'.format(int(B),'bytes' if B > 1 else 'byte')
    elif KB <= B < MB:
        return '{0:.2f} KB'.format(B / KB)
    elif MB <= B < GB:
        return '{0:.2f} MB'.format(B / MB)
    elif GB <= B < TB:
        return '{0:.2f} GB'.format(B / GB)
    elif TB <= B:
        return '{0:.2f} TB'.format(B / TB)

async def track_memory(gdb: GDBCtrl, continue_token: str):
    while True:
        #try:
        breakpt_response = await recv_until_token(gdb, continue_token)
        print(breakpt_response)
        thread_token = await gdb.send('-thread-info 1')
        thread_response = await recv_until_token(gdb, thread_token)
        func = thread_response[-1].results[0].as_native()["threads"][0]["frame"]["func"]
        stack_token = await gdb.send('-stack-list-frames --thread 1' + (" 0 2" if func == "__wrap_free" else " 0 30"))
        stack_response = await recv_until_token(gdb, stack_token)
        print(stack_response)
        args_token = await gdb.send('-stack-list-arguments 1 0 1')
        args_response = await recv_until_token(gdb, args_token)
        print(args_response)
        # interrupt as short as possible
        continue_token = await gdb.send('-exec-continue --thread 1')
        #except Stop
        frames = stack_response[-1].results[0].as_native()["stack"]
        arguments = args_response[-1].results[0].as_native()["stack-args"]
        handle_frames(frames, arguments)


async def main():
    gdb = GDBCtrl()
    await gdb.spawn("gdb-multiarch")
    # -file-exec-and-symbols "/home/ryan/repos/pico-rainviewer/build/pico_rainviewer.elf"
    # -target-select extended-remote localhost:3333
    # -break-insert "/home/ryan/repos/pico-rainviewer/lib/pico-sdk/src/rp2_common/pico_malloc/pico_malloc.c:23" // check_alloc
    # -break-insert "/home/ryan/repos/pico-rainviewer/lib/pico-sdk/src/rp2_common/pico_malloc/pico_malloc.c:65" // realloc
    # -break-insert "/home/ryan/repos/pico-rainviewer/lib/pico-sdk/src/rp2_common/pico_malloc/pico_malloc.c:82" // free
    # -exec-continue --thread 1
    # -thread-info 1
    # -stack-list-frames --thread 1 0 19
    tokens = []
    #print("initializing...")
    init_token = await init(gdb)
    print("Tracking memory...")
    try:
        await track_memory(gdb, init_token)
    except Exception as e:
        print(f"{type(e)}: {e}")
        print(repr(e))
    except asyncio.CancelledError:
        pass
    print("deleting breakpoints...")
    tokens.append(await gdb.send('-break-delete'))
    print_if_token(tokens, await recv_all(gdb))
    print("disconnecting...")
    tokens.append(await gdb.send('-target-disconnect'))
    print_if_token(tokens, await recv_all(gdb))
    print(tokens)
    print(f"There were {len(allocations)} Allocations tracked at exit")
    total = 0
    for allocation in allocations.values():
        print(f"{allocation.creator} held {allocation.size} bytes")
        total += allocation.size
    print(f"At exit, {humanbytes(total)} were allocated on the heap")


if __name__ == "__main__":
    loop = asyncio.get_event_loop()
    main_task = asyncio.ensure_future(main())
    for signal in [SIGINT, SIGTERM]:
        loop.add_signal_handler(signal, main_task.cancel)
    try:
        loop.run_until_complete(main_task)
    finally:
        loop.close()