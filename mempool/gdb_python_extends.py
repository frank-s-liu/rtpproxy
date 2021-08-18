import gdb
import sys
import re

class dt_offs(gdb.Command):
    """dispay typeinfo with all fields name, type and offset.
       Usage: dt class-name
    """
    def __init__(self):
        super (dt_offs, self).__init__ ('dt', gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        if len(argv) != 1:
            raise gdb.GdbError('dt takes exactly 1 argument.')

        stype = gdb.lookup_type(argv[0])

        print argv[0], 'fields'
        for field in stype.fields():
          if not hasattr (field, ('bitpos')):
            print '    =>static: %-30s  : %s' % (field.name, gdb.types.get_basic_type(field.type))
          else:
            print '    =>0x%03x: %-30s  : %s' % (field.bitpos//8,field.name, gdb.types.get_basic_type(field.type))

dt_offs()


def execute_gdb_command(cmd):
    res = ""
    try:
      res = gdb.execute(cmd, False,True)
    except:
      res='invalid'
    return res
    

def get_mem_int_array_value(ptr, count):
    strlines=execute_gdb_command("x /%dx 0x%x"%(count,ptr)).split('\n')
    values = []
    for line in strlines:
        if line:
            values += line.split(':')[1].split()
    return [int(i, 16) for i in values]


def get_mem_int_value(ptr):
    return int(gdb.execute("x /lu {0}".format(ptr), False, True).split(':')[1])

def find_word_value_pos( begin, end, value):
    cmd="find /w 0x%x, +%d, 0x%x"%(begin, end-begin, value)
#    print "excute cmd: %s"%cmd
    addresses = gdb.execute(cmd, False, True).split('\n')
    return [int(a, 0) for a in addresses if a.startswith('0x')]

def get_reg_value(regname):
    val = int(gdb.execute("p /x $%s"%regname, False, True).split('\n')[0].split('=')[1], 0)
#    print "get reg:%s(%d)"%(regname,val)
    return val



def print_ebp_info(ebp_addr, indent):
    up_ebp=get_mem_int_value(ebp_addr)
    eip=get_mem_int_value(ebp_addr+4)
    eip_str=execute_gdb_command("x 0x%x"%eip).split('\n')[0].split(':\t')[0]
    if eip_str=='invalid':
        eip_str='0x%x invalid address'%eip
    print "%s ebp:0x%x, eip:%-80s, up_ebp:0x%x"%(indent, ebp_addr, eip_str, up_ebp)


def get_stack_mem_range_by_ptr(ptr):
    file_info=execute_gdb_command('info files').split('\n')
    file_info_search=filter(lambda i: i.find(("0x%x"%ptr)[0:4])!=-1, file_info)
#    print "files %s"%file_info_search
    for line in file_info_search:
        bounds=filter(lambda i:i!='', re.split(r'[- \t]', line))
#        print "bounds {}".format(bounds)
        lowbound=int(bounds[0],0)
        upbound=int(bounds[1],0)
        if ptr >=lowbound and ptr <= upbound:
            return (lowbound, upbound)
    return 0,0


def search_lower_ebp(esp, upbound, ebp_addr, layer):
    indent_str= reduce(lambda s, i: s+' ', range(layer*4), '') + '|-'
    print_ebp_info(ebp_addr, indent_str)
    low_ebp_addrs=find_word_value_pos(esp, upbound, ebp_addr)
    for j in range(len(low_ebp_addrs)):
        low_ebp=low_ebp_addrs[-j-1]
        search_lower_ebp(esp, upbound, low_ebp, layer+1)


class search_lower_frame(gdb.Command):
    """Reverse search frame(eip+ebp) in stack with given eip or ebp address.
       Usage: sframe [-eip $eip_address] [-ebp $ebp_address]
       Exsample:
       (gdb) sframe -eip 0xf71530de
        esp:0xe8b3bb70 in stack 0xe8a54000 - 0xe8b54000
        find ebps:['0xe8b533e8']
            |- ebp:0xe8b533e8, eip:0xf71530de <clone+94>                                                           , up_ebp:0x0
                |- ebp:0xe8b5335c, eip:0x44ed582 invalid address                                                       , up_ebp:0xe8b533e8
                |- ebp:0xe8b53328, eip:0xf774db4c <start_thread+204>                                                   , up_ebp:0xe8b533e8
                |- ebp:0xe8b532f8, eip:0xf71be157 <res_thread_freeres+23>                                              , up_ebp:0xe8b533e8
                |- ebp:0xe8b532c0, eip:0x0 invalid address                                                             , up_ebp:0xe8b533e8
                |- ebp:0xe8b532a8, eip:0x9372cfe <webrtc::AudioDeviceDummy::PlayThreadProcess()+110>                   , up_ebp:0xe8b533e8
                |- ebp:0xe8b53258, eip:0x93700cf <webrtc::AudioDeviceBuffer::RequestPlayoutData(unsigned int)+223>     , up_ebp:0xe8b533e8
    """
    
    def __init__(self):
        super (search_lower_frame, self).__init__ ('sframe', gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
    
        argv = gdb.string_to_argv(arg)
        if len(argv) < 1:
            raise gdb.GdbError('sframe takes eip or ebp as argument.')
        ref_eip=None
        ref_ebp=None
        if argv[0]=='-eip':
            ref_eip=int(argv[1], 0)
        elif argv[0]=='-ebp':
            ref_ebp=int(argv[1], 0)
        else:
            return

        esp=get_reg_value('esp');
        lowbound,upbound=get_stack_mem_range_by_ptr(esp)
        print "esp:0x%x in stack 0x%x - 0x%x"%(esp, lowbound,upbound)

        ebp_addrs=[]
        if ref_eip:
            addrs=find_word_value_pos(esp, upbound, ref_eip)
            ebp_addrs = map(lambda x:x-4, addrs)
        elif ref_ebp:
            ebp_addrs=find_word_value_pos(esp, upbound, ref_ebp)
        print "find ebps:{}".format(['0x%x' % a for a in ebp_addrs])
        
        for i in range(len(ebp_addrs)):
            ebp_addr = ebp_addrs[-i-1]
            search_lower_ebp(esp, upbound, ebp_addr, 1)


search_lower_frame()



class malloc_dump(gdb.Command):
    """Search and dump struct malloc_trunk info from given addresses.
       Usage: heapdump $address dump_count [,max_chunk_size]
       dump_count: can set with negtive value, it will reverse search up chunks and dump find results.
       max_chunk_size: this is a hint for the max chunk size, if not given, will use default value.
    """
    
    def __init__(self):
        super (malloc_dump, self).__init__ ('heapdump', gdb.COMMAND_DATA)

    def iterator_chunk_forward(self, next_chunk_ptr, count):
        for i in range(count):
            next_chunk_size=get_mem_int_value(next_chunk_ptr+4)
            next_chunk_prev_use_flag=next_chunk_size&0x00000001
            next_chunk_size=next_chunk_size&0xFFFFFFF8
            next_chunk_prev_size=get_mem_int_value(next_chunk_ptr)
            print "next chunk: pos:0x%x, size:%-8d prev_in_use_flag:%d  prevsize:%-12d" % (next_chunk_ptr, next_chunk_size, next_chunk_prev_use_flag, next_chunk_prev_size)
            next_chunk_ptr+=next_chunk_size

    def search_backward_chunk(self,cur_chunk_ptr, max_size):
        for i in range(max_size>>2):
            size_val=get_mem_int_value(cur_chunk_ptr-(i+1)*4)
            size_use_flag=size_val&0x00000001
            size_val=size_val&0xFFFFFFF8
            if size_val<max_size and size_val>=24:
#                print "got chunk size: %d in pos:0x%x prev_size_use_flag:%d" %(size_val,cur_chunk_ptr-(i+1)*4,size_use_flag)
                prev_chunk_ptr=cur_chunk_ptr-(i+1)*4-4
                next_chunk_ptr=prev_chunk_ptr+size_val
#                print "prev chunk:0x%x, cur chunk:0x%x" %(prev_chunk_ptr,cur_chunk_ptr)
                if (next_chunk_ptr == cur_chunk_ptr) or (size_use_flag==1):
                    return (prev_chunk_ptr, size_val, size_use_flag, get_mem_int_value(prev_chunk_ptr))
                elif next_chunk_ptr < cur_chunk_ptr:
	            if size_use_flag == 0:
                        pprev_chunk_size=get_mem_int_value(prev_chunk_ptr)
                        real_pprev_chunk_size=get_mem_int_value(prev_chunk_ptr-pprev_chunk_size+4)&0xFFFFFFF8
#                        print "pprev_size:0x%x real_pprev_size:0x%x"%(pprev_chunk_size,real_pprev_chunk_size)
                        if real_pprev_chunk_size == pprev_chunk_size:
                            return (prev_chunk_ptr, size_val, size_use_flag, pprev_chunk_size)
                    
        return (0, 0, 0, 0)
        
    def search_prev_chunk(self, chunk_ptr, chunk_size, prev_use_flag, max_chunk_size):
        prev_chunk_use_flag=chunk_size&0x00000001
        chunk_size = chunk_size&0xFFFFFFF8
        prev_chunk_size=0
        if prev_chunk_use_flag == 0:
            prev_chunk_size=get_mem_int_value(chunk_ptr)            
#            print "prev chunk not in use, prev chunk size:%d"%prev_chunk_size
            if prev_chunk_size<=max_chunk_size:
                prev_chunk_ptr=chunk_ptr-prev_chunk_size
                real_prev_chunk_size=get_mem_int_value(prev_chunk_ptr+4)
                real_prev_chunk_use_flag=real_prev_chunk_size&0x00000001
                real_prev_chunk_size=real_prev_chunk_size&0xFFFFFFF8
#                print "real prev size %d, prev size %d" %(real_prev_chunk_size,prev_chunk_size)
                if real_prev_chunk_size == prev_chunk_size:
                    return (prev_chunk_ptr,real_prev_chunk_size,real_prev_chunk_use_flag,get_mem_int_value(prev_chunk_ptr))
        #now previos chunk is used or current chunk data is overwrited
        return self.search_backward_chunk(chunk_ptr, max_chunk_size)


    def iterator_chunk_backward(self, chunk_ptr, chunk_size, prev_use_flag, prev_size, count, max_chunk_size):
        print "this chunk: pos:0x%x, size:%-8d, prev_in_use_flag:%d, prev_size:%-12d" % (chunk_ptr,chunk_size,prev_use_flag,prev_size)
        for i in range(count):
            prev_chunk_ptr, prev_chunk_size, pprev_use_flag, pprev_chunk_size = self.search_prev_chunk(chunk_ptr,chunk_size, prev_use_flag, max_chunk_size)
            if prev_chunk_ptr==0:
                print "no more chunk."
                break;
            print "prev_chunk: pos:0x%x, chunk_size:%-8d data_size:%-8d prev_in_use_flag:%d  prevsize:%-12d" %(prev_chunk_ptr,prev_chunk_size, (prev_chunk_size-4 if prev_use_flag else prev_chunk_size-8), pprev_use_flag,pprev_chunk_size)
            (chunk_ptr,chunk_size, prev_use_flag)=(prev_chunk_ptr, prev_chunk_size, pprev_use_flag)
        
        
    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        if len(argv) < 1:
            raise gdb.GdbError('heapdump takes exactly 1 argument.')
        ptr=int(argv[0], 0)
        #lowbound,upbound =get_stack_mem_range_by_ptr(ptr)
        #print "ptr:0x%x in stack 0x%x - 0x%x"%(ptr, lowbound,upbound)
        prev_chunk_size=get_mem_int_value(ptr)
        chunk_size=get_mem_int_value(ptr+4)
        prev_chunk_use_flag=chunk_size&0x00000001
        chunk_size=chunk_size&0xFFFFFFF8

        count = 1
        if (len(argv) > 1):
            count = int(argv[1])
        if count > 0:
            print "this chunk: pos:0x%x, size:%-8d prev_in_use_flag:%d, prev_size:%-12d"%(ptr, chunk_size, prev_chunk_use_flag, prev_chunk_size)
            next_chunk_ptr=ptr+chunk_size
            self.iterator_chunk_forward(next_chunk_ptr, count)
        else:
            max_chunk_size=10240
            if len(argv)>2:
                max_chunk_size=int(argv[2])
            print "iterator backward %d chunks with max_chunk_size:%d" % (-count,max_chunk_size)
            self.iterator_chunk_backward(ptr, chunk_size, prev_chunk_use_flag, prev_chunk_size, -count, max_chunk_size)

        
malloc_dump()


class dump_eip_in_stack(gdb.Command):
    """Dump all symbol addresses in stacks
       Usage: eipdump [max_stack_len]
       max_stack_len : max search length in current thread stack, default is 100K
    """

    def __init__(self):
        super (dump_eip_in_stack, self).__init__ ('eipdump', gdb.COMMAND_DATA)


    def try_print_as_eip(self,addr):
        if addr=='0x00000000':
            return ''
        eip_str=execute_gdb_command("x 0x%x"%addr)
        #print eip_str
        if eip_str.find('<') != -1:
            return eip_str.split('\n')[0].split(':\t')[0]
        return ''

    def invoke(self, arg, from_tty):
        argv = gdb.string_to_argv(arg)
        dump_size = 1024*100
        if (len(argv) > 1):
            dump_size = int(argv[1])
        esp=get_reg_value('esp');
        lowbound,upbound=get_stack_mem_range_by_ptr(esp)
        print "esp:0x%x in stack 0x%x - 0x%x"%(esp, lowbound,upbound)
        addr=esp-160
        while addr < (esp+dump_size) and addr < (upbound-160):
            values=get_mem_int_array_value(addr, 40)
    #        print values
            for i in range(len(values)):
                #print v
                eip_str=self.try_print_as_eip(values[i])
                if eip_str!='':
                    print "ebp:0x%x eip:%s"%(addr+i*4-4,eip_str)
            addr=addr+160

dump_eip_in_stack()

