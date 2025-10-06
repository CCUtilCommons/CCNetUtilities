function(print_banner text)
    string(LENGTH "${text}" text_len)
    math(EXPR line_len "${text_len} + 10")

    string(REPEAT "-" ${line_len} line)

    # 计算左右缩进，使文字居中
    math(EXPR padding "(${line_len} - ${text_len}) / 2")
    string(REPEAT " " ${padding} spaces)

    message("${line}")
    message("${spaces}${text}")
    message("${line}")
endfunction()