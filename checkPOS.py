# coding=utf-8
import sys,os
import struct


def unpackIMU(imuFile):
    buffer = bytes()
    s = struct.Struct('<3xd')
    with open(imuFile,'rb') as f:
        buffer = f.read()
    count = 0
    out = list()
    for i in range(len(buffer)):
        if (buffer[count]==0xaa) and (buffer[count+43]==0xac):
            try:
                a=(int) (((s.unpack_from(buffer,count))[0])*1000)
            except:
                print((((s.unpack_from(buffer,count))[0])*1000))
            count = count +44
            out.append(a)
            #print(a)
        else:
            count = count +1
        
        if(count >= (len(buffer)-50)):
            break;
    return out
  
def unpackDSP(dspFile):
    buffer = bytes()
    s = struct.Struct('<42xd')
    with open(dspFile,'rb') as f:
        buffer = f.read()
    count = 0
    out = list()
    for i in range(len(buffer)):
        if (buffer[count]==0xaa) and (buffer[count+1]==0xac):
            try:
                a=(int) (((s.unpack_from(buffer,count))[0])*1000)
            except:
                print((((s.unpack_from(buffer,count))[0])*1000))
            count = count +74
            out.append(a)
            #print(a)
        else:
            count = count +1
        
        if(count >= (len(buffer)-80)):
            break;
    return out


if __name__ == "__main__":

	if len(sys.argv) == 3:
		print("时标文件:"+(sys.argv)[1])
		print("导航文件:"+(sys.argv)[2])
	else:
		print(sys.argv)

	a=unpackIMU(r'E:\ti_pos\22222222222222222222222\posData\shiBiao.bin')
	count =0
	for i in range(len(a)-1):
	    if (a[i]+8+1)<a[i+1]:
	        count = count +1
	print("IMU checkout::总帧数: {} 丢帧数:{} 丢帧率:{}% 测试总时间:{}s".format(len(a), count, (count*100)/(len(a)),(a[-1]-a[0])/1000))
	
	a=unpackDSP(r'E:\ti_pos\22222222222222222222222\posData\daoHang.bin')
	count =0
	for i in range(len(a)-1):
	    if (a[i]+8+1)<a[i+1]:
	        count = count +1
	print("DSP checkout::总帧数: {} 丢帧数:{} 丢帧率:{}% 测试总时间:{}s".format(len(a), count, (count*100)/(len(a)),(a[-1]-a[0])/1000))
	
