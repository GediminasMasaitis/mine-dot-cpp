
//#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable
//#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable

__kernel void find_combination(const unsigned char map_size, const __constant unsigned char* map, __global unsigned int* results, __global int* increment, const unsigned int offset)
{
	const unsigned int prediction = offset + get_global_id(0);

	unsigned char prediction_valid = 1;
	for(unsigned char i = 0; i < map_size; i++)
	{
#ifndef ALLOW_LOOP_BREAK
		if(!prediction_valid)
		{
			continue;
		}
#endif
		unsigned char neighbours_with_mine = 0;
		const unsigned short header_offset = i * 9;
		const unsigned char header = map[header_offset];
		const unsigned char neighbour_count = header & 0x0F;
		const unsigned char hint = header >> 4;
		for(unsigned char j = 1; j <= neighbour_count; j++)
		{
			const unsigned char neighbour = map[header_offset + j];
			const unsigned char flag = neighbour & 0x03;
			switch(flag)
			{
			case 1:
				++neighbours_with_mine;
				break;
			case 2:
				break;
			default:
			{
				if((prediction & (1 << (neighbour >> 2))) != 0)
				{
					++neighbours_with_mine;
				}
				break;
			}
			}
		}

		if(neighbours_with_mine != hint)
		{
			prediction_valid = 0;
#ifdef ALLOW_LOOP_BREAK
			break;
#endif
		}
	}
	
	if(prediction_valid)
	{
		results[atomic_inc(increment)] = prediction;
	}
}
)