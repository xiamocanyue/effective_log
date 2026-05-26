/*
main（）函数中，使用LogFactory作为整体控制者，使用GetLogHandle获得一个loghandle对象，之后就用这个对象来记录日志。
在main（）函数中，调用info，debug，error等操作，将错误提示先传给VariadicLogHandle，
之后这个类将错误提示和错误场景信息整合，格式化，之后再通过log（），将整合好的错误信息交给loghandle。
loghandle检查日志级别，如果可以输出，就创建一个logmsg对象来保存这个错误信息。
有了logmsg之后，loghanle调用自己的log（），将这个msg发给自己要写的不同的logsink，
每个sink先用format将错误信息logmsg按照写入文本的格式提取出需要的内容，组成输出内容，再加密，压缩，之后就可以向需要的地方写日志了

*/
