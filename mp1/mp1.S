
# mp1.S - missile-command version
# Good luck, and have fun!

.data
	
    # Constants for accessing the fields of a struct missile, 
    # struct missile is defined in rtc.h and mp1.h

	NEXT 	  = 0
	X	  = 4
	Y	  = 8
	VX	  = 12
	VY	  = 16
	DEST_X	  = 20
	DEST_Y	  = 24
	EXPLODED  = 28
	C	  = 32

    # Character to draw for an explosion - '@'

	EXPLOSION = 64


# Data shared between this file and rtc.c helper functions

# This '.globl' directive makes these symbols visible externally
.globl mp1_missile_list, base_alive, mp1_score

mp1_missile_list:	.long 0x0	# Head of list pointer
base_alive:		.long 0x0	# Array of 3 bytes, plus a padding byte
mp1_score:		.long 0x0	# Player's current score


# Data private to this  file

base_pic:     	    .string "/^^^\\"	# Picture of a live base
dead_base_pic: 	    .string "xxxxx"	# Picture of a dead base
crosshairs_x:		.long 0x0	# X-position of the crosshairs
crosshairs_y:		.long 0x0	# Y-position of the crosshairs

.text

# void mp1_poke(void);
# You can use this function to write to video memory.
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX



mp1_poke:
		movl    vmem_base_addr(,1),%edx
		movb    %cl,(%edx,%eax,1)
		ret

#   ----------------- Exported functions ---------------------

# void mp1_rtc_tasklet(unsigned long garbage);
# Performs three tasks:
#	(1) updates the list of missiles (implement this in update_missiles,
#           below, and call it from here).
#	(2) Redraw the bases - they may have been overwritten with missiles
#	(3) Redraw the crosshairs - it may have been overwritten with missiles
# Inputs   : none
# Outputs  : none
# Registers: Standard C calling convention


jump_table:
.long mp1_ioctl_startgame, mp1_ioctl_addmissile, mp1_ioctl_movexhairs, mp1_ioctl_getstatus, mp1_ioctl_endgame

.globl mp1_rtc_tasklet
mp1_rtc_tasklet:
		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %ebx # save the callee-saved registers
		pushl %esi
		pushl %edi

		subl $0,%esp # make space for local variables

		pushl %eax # save the caller-saved registers
		pushl %edx
		pushl %ecx

		# function body：


		# space for push parameters:

		call update_missiles
		call draw_cities
		call draw_crosshair
		

		# space for pop the parameters:

		popl %ecx # restore the caller-saved registers
		popl %edx
		popl %eax

		addl $0,%esp	# tear down the local variables

		popl %edi # restore the callee-saved registers
		popl %esi
		popl %ebx

		leave
		ret 



		
# int mp1_ioctl(unsigned long arg, unsigned int cmd)
# The dispatch function for the MP1 ioctls - should use the cmd argument
# and a jumptable to execute one of the specific ioctls implemented below.
# Inputs   : unsigned long arg - parameter to the mp1_ioctl_....
#          : unsigned int cmd  - specifies which mp1_ioctl_... function 
#          :                     to execute
# Outputs  : Returns an integer - depends on which ioctl() function is called
# Registers: Standard C calling convention

.globl mp1_ioctl
mp1_ioctl:

		# function body：


		movl 8(%esp),%eax # ebx stores the cmd
		cmpl $0,%eax # compare it with 0
		jb invaild_cmd
		cmpl $4,%eax
		ja invaild_cmd

		movl $jump_table, %edx # edx stores the address of the jump table
		imull $4,%eax  # 4*index
		addl %eax,%edx  # store the address the 
		movl (%edx),%edx
		jmp *%edx       # jump to the core function


		# now jump to the corresponding table

invaild_cmd:
		movl $-1,%eax # return the -1 
		ret 


# ----------------- Functions private to this file -------------------

update_missiles:

		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %ebx # save the callee-saved registers
		pushl %esi
		pushl %edi

		subl $0,%esp # make space for local varibales

		# function body stars
		xorl %ebx,%ebx # init the registers
		xorl %eax,%eax
		xorl %ecx,%ecx
		xorl %esi,%esi
		xorl %edi,%edi
		xorl %edx,%edx

		movl mp1_missile_list,%esi # esi store the head of the missile table
		movl $mp1_missile_list,%edi # edi stores the address of the missile table

		cmpl $0,%esi      # check whether it is empty
		je loop_end

check_loop:		
		# walk through the link list to update each missiles
		movl 4(%esi), %ebx  # ebx stores the x   
		sarl $16,%ebx
		movl 8(%esi), %ecx  # ecx stores the y
		sarl $16,%ecx
		

		pushl %ecx
		pushl %ebx
		call calc_offset # calc the offset
		popl %ebx
		popl %ecx

		movb $32,%cl  # store the space to erase
		pushl %edx
		call mp1_poke  # erase
		popl %edx

		
		movl 8(%esi), %ecx  # ecx stores the y
		movl 28(%esi),%ebx # ebx stores the explode
		cmpl $0,%ebx # check whether it is exploding

		jg explode_process


		movl 4(%esi), %ebx  # ebx stores the x
		addl 12(%esi),%ebx  
		addl 16(%esi),%ecx # update the location

		movl %ebx,4(%esi) # update in the struct
		movl %ecx,8(%esi)
map_check:
		# check whether it is in the map
		sarl $16,%ebx
		sarl $16,%ecx

		cmpl $0,%ebx
		jl remove_process
		cmpl $79,%ebx
		jg remove_process   # finishing checking the x range
		
		cmpl $0,%ecx
		jl remove_process
		cmpl $24,%ecx
		jg remove_process   # finishing checking the x range

		# check whether it reaches the destination or is exploding
		movl 20(%esi),%edx  # edx stores the destx
		movl 24(%esi),%eax  # eax stores the desty

		# sarl $16,%edx
		# sarl $16,%eax

		movl 4(%esi), %ebx  # ecx stores the x
		movl 8(%esi), %ecx  # ecx stores the y

		sarl $16,%ebx
		sarl $16,%ecx

		cmpl %ebx,%edx
		je second_dest_check  
not_in_dest:
		movl 28(%esi),%edx  # edx stores the explode
		cmpl $0,%edx
		jne explode_process


		# now the mission is still moving, below to redraw them in the destination
		
		# draw the new location
		movl 4(%esi), %ebx  # ebx stores the x   
		movl 8(%esi), %ecx  # ecx stores the y
		
		sarl $16,%ebx
		sarl $16,%ecx
		pushl %ecx
		pushl %ebx
		call calc_offset # calc the offset
		popl %ebx
		popl %ecx

		movb 32(%esi),%cl  # store the character to fill
		pushl %edx
		call mp1_poke  # erase
		popl %edx


		# now the process of a missile has finished
next_missile:
		movl (%esi),%edx 		
		cmpl $0,%edx      # check whether it is empty
		je loop_end
		movl %esi,%edi    # edi stores the address of last missile
		movl (%esi),%esi  # esi stores the address of current missile
		jmp check_loop
loop_end:

		# function body ends
		addl $0,%esp # tear down the local variables
		
		popl %edi # restore the callee-saved registers
		popl %esi
		popl %ebx

		leave
		ret
		

second_dest_check:
		cmpl %ecx,%eax
		je explode_process
		jmp not_in_dest


remove_process:

		movl (%esi),%ebx

		pushl %edx # save the caller-saved registers
		pushl %ecx

		pushl %edi # push last missile location
		pushl %esi # push current missile location

		call mp1_missile_remove

		popl %esi
		popl %edi

		popl %ecx # restore the caller-saved registers
		popl %edx
		
		# movl (%eax),%ebx 		
		cmpl $0,%ebx      # check whether it is empty
		je loop_end
					      # edi stores the address of last missile
		movl %ebx,%esi    # esi   stores the address of current missile
		jmp check_loop

mp1_missile_remove:
		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %ebx # save the callee-saved registers
		pushl %esi
		pushl %edi

		subl $0,%esp # make space for local varibales

		# function body stars
		xorl %ebx,%ebx # init the registers
		xorl %eax,%eax
		xorl %ecx,%ecx
		xorl %esi,%esi
		xorl %edi,%edi
		xorl %edx,%edx

		movl 8(%ebp),%esi # load the parameters
		movl 12(%ebp),%edi 
		movl (%esi),%ecx  # link the last element's next to current element's next
		movl %ecx,(%edi)

		pushl %edx # save the caller-saved registers
		pushl %ecx

		pushl %esi
		call mp1_free
		popl %esi

		popl %ecx # restore the caller-saved registers
		popl %edx
		# function body ends

		addl $0,%esp # tear down the local variables
		
		popl %edi # restore the callee-saved registers
		popl %esi
		popl %ebx

		leave
		ret

explode_process:

		pushl %edx # save the caller-saved registers
		pushl %ecx

		pushl %esi # push current missile location

		call missile_explode

		popl %esi

		popl %ecx # restore the caller-saved registers
		popl %edx
		
		cmpl $0,%eax # check whether the status has changed
		je skip_notice
		pushl %edx
		pushl %ecx
		call mp1_notify_user # call the notice block
		popl %ecx
		popl %edx
		jmp skip_notice

calc_offset:  # return the offset in %eax
		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %esi
		pushl %edi

		movl 8(%ebp),%edi # load the parameter the x to paint
		movl 12(%ebp),%esi # load the parameter the y to paint
		imull $160,%esi,%esi # esi = y*160
		imull $2,%edi,%edi   # 2x
		addl %esi,%edi

		movl %edi,%eax

		popl %edi # restore the callee-saved registers
		popl %esi
		

		leave
		ret

skip_notice:
		movl 28(%esi),%edx
		subl $1,%edx
		movl %edx,28(%esi)
		cmpl $0,%edx
		je remove_process
		movl $64,32(%esi)

		# draw the new location
		movl 4(%esi), %ebx  # ebx stores the x   
		movl 8(%esi), %ecx  # ecx stores the y
		
		sarl $16,%ebx
		sarl $16,%ecx

		pushl %ecx
		pushl %ebx
		call calc_offset # calc the offset
		popl %ebx
		popl %ecx

		movb 32(%esi),%cl  # store the @ to paint
		pushl %edx
		call mp1_poke  # erase
		popl %edx

		jmp next_missile

draw_crosshair:
		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %esi
		pushl %edx
		pushl %ecx

		movl $crosshairs_x,%esi
		movl $crosshairs_y,%edx
		movl (%esi),%esi
		movl (%edx),%edx

		pushl %edx
		pushl %esi # push the x

		call calc_offset

		popl %esi
		popl %edx

		movb $43,%cl

		pushl %edx
		call mp1_poke
		popl %edx
		
		popl %ecx
		popl %edx
		popl %esi
		leave
		ret

draw_cities:
		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %esi
		pushl %edx
		pushl %ecx

		xorl %edi,%edi # initialize the counter
		movl $18,%edx # edx now stores the x value of the first city
		movl $24,%eax # eax stores the y value of all the cities
		movl $base_alive, %esi # ecx stores the address the first city
		
		# function body starts
judge_city:
		addl $1,%edi # increase the counter by 1
		cmpl $4,%edi # check whether it has reached all
		je judge_city_done 
		movb (%esi),%cl # esi stores the check bits for the city
		cmpb $0,%cl # check the city
		je death_judge # it should be dead
		jmp live_judge # or it goes to live
death_judge:
		movl $dead_base_pic,%ebx
		pushl %eax
		pushl %edx
		pushl %ecx

		pushl %ebx
		pushl %eax # push y
		pushl %edx # push x

		call draw_cities_cha

		# addl $1,%edi # city counter increase by 1

		popl %edx # push x 
		popl %eax # push y
		popl %ebx 

		popl %ecx
		popl %edx
		popl %eax

		addl $20,%edx
		addl $1,%esi

		jmp judge_city

live_judge: 
		movl $base_pic,%ebx

		pushl %eax
		pushl %edx
		pushl %ecx

		pushl %ebx
		pushl %eax # push y
		pushl %edx # push x

		call draw_cities_cha

		# addl $1,%edi

		popl %edx
		popl %eax # push y
		popl %ebx # push x 

		popl %ecx
		popl %edx
		popl %eax

		addl $20,%edx
		addl $1,%esi

		jmp judge_city

judge_city_done:
		# function body ends
		
		popl %ecx
		popl %edx
		popl %esi
		leave
		ret

draw_cities_cha:

		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %esi
		pushl %ebx
		pushl %edi

		movl 8(%ebp),%esi # esi stores the X
		movl 12(%ebp),%edx # edx stores the y
		movl 16(%ebp),%ebx # ebx stores the address of the first character
		movb (%ebx),%cl # cl stores the address of the first character to print

		# function body starts
		xorl %edi,%edi # initialize the counter
draw_loop:
		pushl %edx			
		pushl %esi
		call calc_offset
		popl %esi
		popl %edx

		pushl %edx
		call mp1_poke
		popl %edx

		addl $1,%esi # update the x
		addl $1,%edi # update the counter
		addl $1,%ebx # go to the location
		movb (%ebx),%cl # cl stores the address of the first character to print
		cmpl $5,%edi # check whether it is done
		je draw_loop_down
		jmp draw_loop

draw_loop_down:
		

		# function body ends
		
		popl %edi
		popl %ebx
		popl %esi
		leave
		ret



mp1_ioctl_startgame:

		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %esi
		pushl %ebx
		pushl %edi

		movl $base_alive,%edx
		movb $1,(%edx)
		movb $1,1(%edx)
		movb $1,2(%edx) 
		movl $mp1_score,%edx
		movl $0,(%edx)
		movl $mp1_missile_list,%edx
		movl $0,(%edx)

		movl $crosshairs_x,%edx
		movl $crosshairs_y,%ecx
		movl $40, (%edx)			# X-position of the crosshairs
		movl $12, (%ecx)			# Y-position of the crosshairs

		# movl $40,%ebx
		# movl $12,%esi

		# pushl %esi
		# pushl %ebx
		# call calc_offset
		# popl %ebx
		# popl %esi
		
		# movb $43,%cl

		# pushl %edx
		# call mp1_poke
		# popl %edx

		pushl %edi
		pushl %ebx
		pushl %esi

		leave
		ret

mp1_ioctl_addmissile:

		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %esi  # callee saved register
		pushl %ebx
		pushl %edi

		movl 8(%ebp),%esi # store the from pointer to esi
		movl $36,%edx # the space to allocate



	
		 # caller saved
		pushl %ecx
		pushl %edx
		pushl %edx # pop parameter
		call mp1_malloc

		popl %edx # pop parameter
		popl %edx # pop parameter
		popl %ecx # pop caller-saved

		# now the space has been created

		cmpl $0,%eax # check whether the pointer is null
		je fail_return

		movl %eax, %ebx # ebx stores the to pointer

		pushl %ecx

		pushl %edx # push parameter
		pushl %esi
		pushl %ebx	
	
		call mp1_copy_from_user

		popl %ebx
		popl %esi
		popl %edx

		popl %ecx
		
		cmpl $0,%eax
		jne fail_return



		movl mp1_missile_list,%edx # edx store the head of the missile table
		# movl $mp1_missile_list,%edi # edi stores the address of the missile table
		movl %edx,(%ebx) # let esi points to the old head
		movl %ebx,mp1_missile_list   # the head of the list is the new missile
		

		movl $0,%eax  # return the ture value
		jmp done_copying

fail_return:
		pushl %edx
		pushl %ecx
		
		pushl %ebx
		call mp1_free
		popl %ebx

		pushl %ecx
		pushl %edx

		movl $-1,%eax # return the fail
		
done_copying:

		pushl %esi
		pushl %ebx
		pushl %edi

		leave
		ret

mp1_ioctl_movexhairs:
		pushl %ebp # save old base pointer
		movl %esp,%ebp

		movl 8(%ebp),%edx # edx stores the 32-bits argument
		movl 8(%ebp),%eax # eax stores the 32-bits argument

		pushl %esi
		pushl %ebx
		pushl %edi

		movl $crosshairs_x,%esi
		movl $crosshairs_y,%ecx

		movl (%esi),%edi # edi store the x
		movl (%ecx),%ebx # ebx store the y

		pushl %ebx    # erase the origin crosshair
		pushl %edi
		call calc_offset  # eax stores the offset
		popl %edi
		popl %ebx

		movb $32,%cl
		
		pushl %edx
		call mp1_poke
		popl %edx

		movl %edx,%eax          # because eax and ecx should be valued again
		movl $crosshairs_x,%esi
		movl $crosshairs_y,%ecx

		shll $16,%edx 
		sarl $16,%edx # edx is the vx
		sarl $16,%eax # eax is the vy

		addl %edx,%edi # the edi stores the new x
		addl %eax,%ebx # the ebx stores the new y
check_x_lower:
		cmpl $0,%edi # check the 0 and x
		jge check_x_upper
		movl $0,%edi
check_x_upper:
		cmpl $79,%edi # check the 79 and x
		jle check_y_lower
		movl $79,%edi
check_y_lower:
	    cmpl $0,%ebx # check the 0 and y
		jge check_y_upper
		movl $0,%ebx
check_y_upper:
		cmpl $24,%ebx # check the 79 and y
		jle move_crosshair
		movl $24,%ebx
move_crosshair:
		movl %edi, (%esi) # store them into the address
		movl %ebx, (%ecx)

		# now draw the new crosshair
		pushl %ebx
		pushl %edi
		call calc_offset  # eax stores the offset
		popl %edi
		popl %ebx
		movb $43,%cl

		pushl %edx
		call mp1_poke
		popl %edx

		popl %edi
		popl %ebx
		popl %esi

		leave
		ret

mp1_ioctl_getstatus:
		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %esi
		pushl %ebx
		pushl %edi

		pushl $4
		call mp1_malloc
		popl %edx

		# eax stores the pointer
		# cmpl $0,%eax # check whether it is a null pointer
		# je copy_fails
		movl %eax,%edx # edx stores the to pointer

		movl 8(%ebp),%esi # esi stores the address of int
		
		movl mp1_score,%edi #edi stores the score
		movl $base_alive, %ecx # ecx stores the address the first city

		xorl %ebx,%ebx
		movb (%ecx),%al # al stores the check value
		cmpb $0,%al 
		je B_city
		addl $1,%ebx
B_city:
		shll $1,%ebx
		addl $1,%ecx 
		movb (%ecx),%al # eax stores the check value
		cmpb $0,%al 
		je C_city
		addl $1,%ebx
C_city:
		# addl (%ecx),%ebx
		shll $1,%ebx
		addl $1,%ecx 
		movb (%ecx),%al # eax stores the check value
		cmpb $0,%al 
		je end_city
		addl $1,%ebx   # now the bits has been loaded into the ebx
end_city:
		shll $16,%ebx  # shift them to the 16,17,18 bits
		addl %edi,%ebx # now all the bits are loaded
		movl %ebx,(%edx) # store it to the buffer

		# pushl $4
		# pushl %edx
		# pushl %esi

		jmp getstatus_end

copy_fails:
		movl $-1,%eax # the return value is -1 if it fails
		pushl %edx
		call mp1_free
		popl %edx

		jmp done_end

getstatus_end:
		pushl $4
		pushl %edx
		pushl %esi
		call mp1_copy_to_user
		popl %esi
		popl %edx
		popl %edi

		cmpl $0,%eax
		jne copy_fails

		movl $0,%eax

done_end:
		popl %edi
		popl %ebx
		popl %esi

		leave
		ret

mp1_ioctl_endgame:
		pushl %ebp # save old base pointer
		movl %esp,%ebp

		pushl %ebx # store the callee-saved registers
		pushl %esi
		pushl %edi


		movl mp1_missile_list,%esi # esi store the head of the missile table
		movl $mp1_missile_list,%edi # edi stores the address of the head 

remove_loop_starts:		
		# walk through the link list to update each missiles

		movl (%esi),%ebx

		pushl %edx # save the caller-saved registers
		pushl %ecx

		pushl %edi # push last missile location
		pushl %esi # push current missile location

		call mp1_missile_remove

		popl %esi
		popl %edi

		popl %ecx # restore the caller-saved registers
		popl %edx
		
		# movl (%eax),%ebx 		
		cmpl $0,%ebx      # check whether it is empty
		je remove_loop_end
					      # edi stores the address of last missile
		movl %ebx,%esi    # esi   stores the address of current missile
		jmp remove_loop_starts

remove_loop_end:

		# function body ends
		
		
		popl %edi # restore the callee-saved registers
		popl %esi
		popl %ebx

		movl $1,%eax # return success
		leave
		ret




