#pragma OPENCL EXTENSION cl_khr_local_int32_extended_atomics : enable
#pragma OPENCL EXTENSION cl_khr_global_int32_extended_atomics : enable

#define INT_MAX 4294967296

// We'll assume this is sufficiently random for the purposes of the simulation.
float random( uint x ) {
    uint value = x;
    value = (value ^ 61) ^ (value>>16);
    value *= 9;
    value ^= value << 4;
    value *= 0x27d4eb2d;
    value ^= value >> 15;
    return (float) value / (float) INT_MAX;
}

__kernel void elect(global const float3 *voters, global uint2 *outcome) {
    
    size_t id = get_global_id(0);
    int a_wins = 0;
    int b_wins = 0;

    for(int i = 0; i < 100000; i++){

        float3 v = voters[i];
        float P = random(RANDOM_SEED + id + i);
        
        if (P < v.x){
            a_wins ++;
        }
        else if (v.x <= P && P < (v.x + v.y) ){
            b_wins ++;
        }
        else if (P >= (v.x + v.y)){
            // do nothing
        }
    }

    outcome[id] = (uint2) {a_wins, b_wins};

}