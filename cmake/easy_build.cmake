function(add_easy_c_executable exe_name)
    message("Configuring the ${exe_name}.c executable")
    add_executable(${exe_name} ${exe_name}.c)    
endfunction()

function(add_easy_cpp_executable exe_name)
    message("Configuring the ${exe_name}.cpp executable")
    add_executable(${exe_name} ${exe_name}.cpp)    
endfunction()

