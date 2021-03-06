#ifdef _WIN32
#include <Windows.h>
#else
#include <sys/mman.h>
#endif

#include "../errors.hpp"
#include "jitcache.hpp"

JitCache::JitCache()
{
    //We reserve blocks so that they don't get reallocated.
    blocks.reserve(1024 * 4);
    current_block = nullptr;
}

//Allocate a block with read and write, but not executable, privileges.
void JitCache::alloc_block(BlockState state)
{
    JitBlock new_block;

    new_block.mem = nullptr;
    new_block.block_start = nullptr;
    new_block.state = state;
#ifdef _WIN32
    //Errors::die("[JIT] alloc_block not implemented for WIN32");
    new_block.block_start = (uint8_t *)VirtualAlloc(NULL, BLOCK_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    new_block.block_start = (uint8_t*)mmap(NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif

    if (!new_block.block_start)
        Errors::die("[JIT] Unable to allocate block");

    new_block.mem = new_block.block_start;

    new_block.pool_start = &new_block.block_start[START_OF_POOL];
    new_block.pool_size = 0;

    if (blocks.size() > 0x2000)
        flush_all_blocks();

    blocks.insert({ state, new_block });
    current_block = &blocks[state];
}

void JitCache::free_block(BlockState state)
{
    auto search = blocks.find(state);

    if (search == blocks.end()) 
        return;
    if (&search->second == current_block)
        current_block = nullptr;

#ifdef _WIN32
    VirtualFree(search->second.block_start, 0, MEM_RELEASE);
#else
    munmap(search->second.block_start, BLOCK_SIZE);
#endif
    blocks.erase(search);
}

void JitCache::flush_all_blocks()
{
    //Deallocate all virtual memory claimed by blocks
    for (auto it = blocks.begin(); it != blocks.end(); ++it)
    {
#ifdef _WIN32
        //Errors::die("[JIT] flush_all_blocks not implemented for WIN32");
        VirtualFree(it->second.block_start, 0, MEM_RELEASE);
#else
        munmap(it->second.block_start, BLOCK_SIZE);
#endif
    }

    //Clear out the blocks vector and replace it with an empty one.
    //We reserve blocks to prevent them from getting reallocated
    blocks = std::unordered_map<BlockState, JitBlock, BlockStateHash>();
    blocks.reserve(1024 * 4);
    current_block = nullptr;
}

JitBlock *JitCache::find_block(BlockState state)
{
    auto search = blocks.find(state);

    if (search != blocks.end())
    {
        //printf("[VU_JIT64] Block found at %p\n", &(search->second));
        current_block = &(search->second);
        return current_block;
    }
    //printf("[VU_JIT64] Block not found\n");
    return nullptr;
}

uint8_t* JitCache::get_current_block_start()
{
    return current_block->block_start;
}

uint8_t* JitCache::get_current_block_pos()
{
    return current_block->mem;
}

void JitCache::set_current_block_pos(uint8_t *pos)
{
    current_block->mem = pos;
}

//Replace a block's write privileges with executable privileges.
//This is to prevent the security risks that RWX memory has.
void JitCache::set_current_block_rx()
{
#ifdef _WIN32
    //Errors::die("[JIT] set_current_block_rx not implemented for WIN32");
    DWORD old_protect;
    bool pass = VirtualProtect(current_block->block_start, BLOCK_SIZE, PAGE_EXECUTE_READ, &old_protect);
    if(!pass)
        Errors::die("[JIT] set_current_block_rx failed");
#else
    int error = mprotect(current_block->block_start, BLOCK_SIZE, PROT_READ | PROT_EXEC);
    if (error == -1)
        Errors::die("[JIT] set_current_block_rx failed");
#endif
}

void JitCache::print_current_block()
{
    uint8_t* ptr = current_block->block_start;
    uint8_t* end = current_block->mem;
    printf("[JIT Cache] Block: ");
    while (ptr != end)
    {
        printf("%02X ", *ptr);
        ptr++;
    }
    printf("\n");
}

void JitCache::print_literal_pool()
{
    int offset = 0;
    while (offset < current_block->pool_size)
    {
        printf("$%02X ", current_block->pool_start[offset]);
        offset++;
        if (offset % 16 == 0)
            printf("\n");
    }
}
