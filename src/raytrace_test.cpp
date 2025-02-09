/*
Copyright (c) 2018 Paul Stahr

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <inttypes.h>
#include <vector>
#include <numeric>
#include <fstream>
#include <iostream>
#include "io_util.h"
#include "image_util.h"
#include "types.h"
#include "serialize.h"

template <uint64_t divisor>
struct print_div_struct
{
    print_div_struct(){}
    
    template <typename T>
    std::ostream & operator ()(std::ostream & out, T elem) const
    {
        return out << static_cast<double>(elem) / divisor;
    }
};

template <uint64_t divisor>
static const print_div_struct<divisor> print_div;

template<typename IOR_TYPE, typename IORLOG_TYPE, typename DIR_TYPE, typename DIFF_TYPE>
void scaling_test()
{
    Options opt;
    opt._loglevel = -3;
    std::cout << "scaling_test" << std::endl;
    RaytraceInstance<IOR_TYPE, DIR_TYPE> inst;
    inst._bound_vec = std::vector<size_t>({1000,10,10});
    size_t num_pixel = std::accumulate(inst._bound_vec.begin(), inst._bound_vec.end(), size_t(1), std::multiplies<size_t>());
    inst._ior = std::vector<IOR_TYPE>(num_pixel, 0);
    inst._translucency = std::vector<uint32_t>(num_pixel, 0xFFFFFFFF);
    inst._start_position = std::vector<pos_t>({0x10000,0x40000,0x40000,static_cast<pos_t>(0x10000*inst._bound_vec[0] - 0x30000),0x40000,0x40000});
    DIR_TYPE xdir = 0x10 * (std::is_same<DIR_TYPE, dir_t>() ? 0x100 : 0x1);
    inst._start_direction = std::vector<DIR_TYPE>({xdir,0,0, static_cast<DIR_TYPE>(-xdir), 0, 0});
    inst._invscale = std::vector<float>(3,2);
    inst._iterations = 1000000;
    inst._trace_path = true;
    inst._minimum_brightness = 0;
    size_t num_layer_pixel = inst._bound_vec[1] * inst._bound_vec[2];
    if (std::is_same<IOR_TYPE, ior_t>()){ 
        std::fill(inst._ior.begin(), inst._ior.begin() + 10 * num_layer_pixel, 0x10000);
        std::fill(inst._ior.end() - 10 * num_layer_pixel, inst._ior.end(), 0x10000 * 2);
    }
    else
    {
        std::fill(inst._ior.begin(), inst._ior.begin() + 10 * num_layer_pixel, 1);
        std::fill(inst._ior.end() - 10 * num_layer_pixel, inst._ior.end(), 2);
    }
    for (size_t i = 10; i < inst._bound_vec[0] - 10; ++i)
    {
        IOR_TYPE fill_value;
        if (std::is_same<IOR_TYPE, ior_t>()){fill_value = 0x10000 + 0x10000 *          i  / (inst._bound_vec[0] - 21);}
        else                                {fill_value = 1       + static_cast<float>(i) / (inst._bound_vec[0] - 21);}
        std::fill(inst._ior.begin() + i * num_layer_pixel, inst._ior.begin() + (i + 1) * num_layer_pixel, fill_value);
    }
    
    std::vector<pos_t> end_position;
    std::vector<DIR_TYPE> end_direction;
    std::vector<uint32_t> end_iteration;
    std::vector<uint32_t> remaining_light;
    std::vector<pos_t> path;    
    print_elements(std::cout << " beginposition ", inst._start_position.begin(), inst._start_position.end(), ' ', print_div<0x10000>) << std::endl;
    print_elements(std::cout << " begindirection ", inst._start_direction.begin(), inst._start_direction.end(), ' ', print_div<0x100>) << std::endl;
    trace_rays<IOR_TYPE, IORLOG_TYPE, DIFF_TYPE, DIR_TYPE>(
        inst,
        end_position,
        end_direction,
        end_iteration,
        remaining_light,
        path,
        opt);
    std::cout << "scaling " << static_cast<double>(end_direction[0]) / inst._start_direction[0] << ' ' << static_cast<double>(end_direction[3]) / inst._start_direction[3] << ' ' << (static_cast<double>(end_direction[0]) / (inst._start_direction[0] * 2) + inst._start_direction[3] / static_cast<double>(end_direction[3])) * 0.5 << std::endl;
    print_elements(std::cout << " endposition ", end_position.begin(), end_position.end(), ' ', print_div<0x10000>) << std::endl;
    print_elements(std::cout << " enddirection ", end_direction.begin(), end_direction.end(), ' ', print_div<std::is_same<DIR_TYPE, dir_t>() ? 0x100 : 1>) << std::endl;
    print_elements(std::cout << " enditeration ", end_iteration.begin(), end_iteration.end(), ' ') << std::endl;
    for (size_t i = 0; i < 2; ++i)
    {
        std::cout << "path " << ' ';
        for (size_t j = 0; j < 50; ++j)
        {
            auto elem = path.rbegin() + ((1 - i) * inst._iterations + (j * end_iteration[i] / 50)) * 3;
            print_elements(std::cout, elem, elem + 3, ' ', print_div<0x10000>) << ' ';
        }
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[])
{
    RaytraceInstance<ior_t, dir_t> inst;
    if (argc >= 3)
    {
        RayTraceSceneInstance<ior_t> scene_inst;
        RayTraceRayInstance<dir_t> ray_inst;
    
        {
            std::cout << "scene" << std::endl;
            std::ifstream stream(argv[1]);
            SERIALIZE::read_value(stream, scene_inst);
            stream.close();
        }

        {
            std::cout << "ray" << std::endl;
            std::ifstream stream(argv[2]);
            SERIALIZE::read_value(stream, ray_inst);
            stream.close();
        }
        std::cout << ray_inst._invscale.size() << std::endl;
        std::cout << ray_inst._invscale[0] << std::endl;
        Options opt;
        opt._loglevel = -4;
        RaytraceScene<ior_t, iorlog_t, diff_t> scene(scene_inst, opt);
        
        std::vector<pos_t> end_position;
        std::vector<dir_t> end_direction;
        std::vector<uint32_t> end_iteration;
        std::vector<uint32_t> remaining_light;
        std::vector<pos_t> path;
        
        scene.trace_rays(RayTraceRayInstanceRef<dir_t>(ray_inst), end_position, end_direction, end_iteration, remaining_light, path, opt);
        
        print_elements(std::cout << "begin ", ray_inst._start_position.begin(), ray_inst._start_position.end(), ' ') << std::endl;
        print_elements(std::cout << "end ", end_position.begin(), end_position.end(), ' ') << std::endl;
        return 0;
    }
    if (argc >= 2)
    {
        if (std::string("#s") == argv[1])
        {
            scaling_test<ior_t, iorlog_t, dir_t, diff_t>();
            scaling_test<float, float, float, float>();
            return 0;
        }
        
        std::ifstream stream(argv[1]);
        SERIALIZE::read_value(stream, inst);
    }
    else
    {
        inst._bound_vec = std::vector<size_t>({100,100,100});
        inst._ior = std::vector<ior_t>(1000000);
        inst._translucency = std::vector<uint32_t>(1000000);
        inst._start_position = std::vector<pos_t>(10000);
        inst._start_direction = std::vector<dir_t>({1,0,0});
        inst._iterations = 100;
        inst._trace_path = true;
        inst._minimum_brightness = 100;
        
    }
    size_t dim = inst._bound_vec.size();
    std::vector<pos_t> end_position;
    std::vector<dir_t> end_direction;
    std::vector<uint32_t> end_iteration;
    std::vector<uint32_t> remaining_light;
    std::vector<pos_t> path;
    print_elements(std::cout << "bounds: ", inst._bound_vec.begin(), inst._bound_vec.end(), ' ') << std::endl;
    trace_rays<ior_t, iorlog_t, diff_t, dir_t>(inst, end_position, end_direction, end_iteration, remaining_light, path, Options());
    if (end_position.size() < 1000)
    {
        print_elements(std::cout << "end position:", end_position.begin(), end_position.end(),' ') << std::endl;
        print_elements(std::cout << "end direction:", end_direction.begin(), end_direction.end(),' ') << std::endl;
    }
    std::cout << "path" << std::endl;
    if (inst._trace_path)
    {
        for (size_t i = 0; i < inst._iterations; ++i)
        {
            print_elements(std::cout << i << ':', path.begin() + i * dim, path.begin() + (i + 1) * dim, ' ');
            print_elements(std::cout << " <-> ", path.begin() + i * dim, path.begin() + (i + 1) * dim, ' ', print_div<0x10000>) << std::endl;
        }
    }
    
    return 0;
}
