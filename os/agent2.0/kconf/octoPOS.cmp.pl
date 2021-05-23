use File::Find::Rule;

$VAR1 = {
	'vname' => 'octoPOS Family model',
	'name' => 'octoPOS',
	'dir' => '',
	'comp' => [
		{
			'vname' => 'Top-level Standard Include Files',
			'name' => 'ciao_base_stdinc',
			'files' => [
				'os/krn/main.cc',
				'os/krn/main_ilet.h',
				'os/krn/System.h',
				'os/krn/System.cc',
				'os/krn/Types.h',
				'os/krn/Error.h',
				'os/krn/Error.cc',
			],
		},
		{
			'vname' => 'Scripts for building process etc.',
			'name' => 'build_scripts',
			'subdir' => 'scripts',
			'files' => [
				'opaque_typeinfo.pl',
				'octopos_h.pl',
			],
		},
		{
			'vname' => 'Configuration',
			'generate' => 'generate_attribute_file();',
			'file' => 'cfAttribs.h'
		},
		{
			'vname' => 'Network Support',
			'dir' => 'os/net',
			'depends' => '&platform_has_iotile && &os_net',
			'files' => [
				'IP.h',
				'IP.cc',
			],
			'comp' => [
				{
					'vname' => 'Network Device Interface',
					'depends' => '&platform_tile_is_iotile && &os_net_device',
					'files' => [
						'Device.h',
						'Device.cc',
						'Device.ah',
						'Packet.h',
					],
				},
				{
					'vname' => 'TCP Header',
					'subdir' => 'TCP',
					'depends' => '&platform_has_iotile && &os_net_tcp',
					'files' => [
						'../TCP.h',
						'Accept.h',
						'Bind.h',
						'Close.h',
						'Connect.h',
						'Create.h',
						'Listen.h',
						'Send.h',
						'Shutdown.h',
						'Receive.h',
					],
					'comp' => [
						{
							'name' => 'TCP Implementation',
							'depends' => '&platform_tile_is_iotile',
							'files' => [
								'Accept.cc',
								'Bind.cc',
								'Buffer.h',
								'Close.cc',
								'Connect.cc',
								'Create.cc',
								'Crosstile.h',
								'Listen.cc',
								'Listener.cc',
								'Listener.h',
								'PCB.h',
								'Receive.cc',
								'Receiver.cc',
								'Receiver.h',
								'Send.cc',
								'Sender.cc',
								'Sender.h',
								'Shutdown.cc',
								'Socket.h',
								'Socket.cc',
							],
						},
					],
				},
				{
					'vname' => 'Backend: Socket-API',
					'depends' => '&platform_tile_is_iotile && &os_net_backend_socket',
					'comp' => [
						{
							'name' => 'TCP Backend',
							'depends' => '&os_net_tcp',
							'subdir' => 'TCP/Backend',
							'srcdir' => 'os/net/Backend_Socket/TCP',
							'files' => [
								'PCB.cc',
								'PCB.h',
								'Buffer.h',
							],
						},
					],
				},
				{
					'vname' => 'Linux rbtree',
					'comp' => [
						{
							'name' => 'Linux rbtree Make infrastructure',
							'dir' => 'mk',
							'srcfile' => 'linux_rbtree.mk',
							'file' => 'gen/linux_rbtree.mk',
						}
					],
				},
				{
					'vname' => 'Backend: lwIP',
					'depends' => '&platform_tile_is_iotile && &os_net_backend_lwip',
					'comp' => [
						{
							'name' => 'Backend',
							'subdir' => 'Backend',
							'srcdir' => 'os/net/Backend_lwIP',
							'files' => [
								'lwIP.cc',
								'lwIP',
							],
						},
						{
							'name' => 'Device driver',
							'depends' => '&os_net_device',
							'subdir' => 'Backend',
							'srcdir' => 'os/net/Backend_lwIP',
							'files' => [
								'Device.cc',
								'Device.h',
								'Packet.cc',
								'Packet.h',
							],
						},
						{
							'name' => 'TCP Backend',
							'subdir' => 'TCP/Backend',
							'srcdir' => 'os/net/Backend_lwIP/TCP',
							'depends' => '&os_net_tcp',
							'files' => [
								'PCB.cc',
								'PCB.h',
								'Buffer.cc',
								'Buffer.h',
							],
						},
						{
							'name' => 'Library files',
							'dir' => 'lwip',
							'files' => [File::Find::Rule->file->relative->name("*.h", "*.mk", "*.c")->in ($srcp ."/lwip/")],
						},
						{
							'name' => 'Library configuration',
							'dir' => 'lwip/include',
							'comp' => [
								{
									'subdir' => 'arch',
									'srcdir' => 'os/net/Backend_lwIP/lib/arch',
									'files' => ['cc.h',  'sys_arch.h'],
								},
								{
									'srcdir' => 'os/net/Backend_lwIP/lib/lwip',
									'subdir' => 'lwip',
									'file' => 'lwipopts.h',
								},
							],
						},
						{
							'name' => 'Make infrastructure',
							'dir' => 'mk',
							'srcfile' => 'lwIP.mk',
							'file' => 'gen/lwIP.mk',
						}
					],
				},
			],
		},
		{
			'vname' => 'System-call layer',
			'name' => 'os_syscall_meta',
			'dir' => 'os/syscall',
			'comp' => [
				{
					'vname' => 'Enable system-call layer',
					'name' => 'os_syscall',
					'depends' => '&os_syscall',
					'files' => [
						'BinarySignal.h',
						'Claim.h',
						'ClaimTag.h',
						'ContextManager.h',
						'CPU.h',
						'Die.h',
						'DispatchClaim.h',
						'ExecutionManager.h',
						'FibreLocalStorage.h',
						'Future.h',
						'iLet.h',
						'InfectSignal.h',
						'iNoC.h',
						'InvasiveScheduler.h',
						'ProxyClaim.h',
						'Module.h',
						'ResourceManager.h',
						'SimpleSignal.h',
						'Stopwatch.h',
						'Tile.h',
						'WorldStopper.h',
					],
				},
				{
					'vname' => 'Include i-let id & application-class in system-call layer',
					'name' => 'os_syscall_ilet_appClass',
					'depends' => '&os_syscall && &os_proc_ilet_applClass',
					'files' => [
						'ApplicationClass.h',
						'CiC.h',
						'iLet_AppClass.ah',
						'iLet_AppClass_Slice.ah',
					],
				},
				{
					'vname' => 'System-call wrapper for TCPA interface',
					'name' => 'os_syscall_TCPAProxyClaim',
					'depends' => '&os_syscall && &os_res_tcpaProxyClaim',
					'files' => [
						'TCPAProxyClaim.h',
					]
				},
				{
					'vname' => 'System-call wrapper for VGA interface',
					'name' => 'os_syscall_vga',
					'depends' => '&os_syscall && &os_dev_vga',
					'files' => [
						'VGA.h',
					],
				},
				{
					'vname' => 'System-call wrapper for clock functionality',
					'name' => 'os_syscall_clock',
					'depends' => '&os_syscall && &os_dev_wallClock',
					'files' => [
						'WallClock.h',
					],
				},
				{
					'vname' => 'System-call wrapper for Events',
					'name' => 'os_syscall_event',
					'depends' => '&os_syscall && &os_res_event',
					'file' => 'Event.h'
				}
			],
		},
		{
			'vname' => 'Binding for C-Applications into os::krn::System::appl_hook()',
			'name' => 'os_krn_cAppBinding',
			'depends' => '&os_krn_cAppBinding',
			'files' => [
				'os/krn/CApplicationBinding.h',
				'os/krn/CApplicationBinding.cc',
			],
		},
		{
			'vname' => 'OS Info',
			'dir' => '/os/krn',
			'files' => [
				'OSInfo.h',
				'OSInfo.cc',
			],
		},
		{
			'vname' => 'Boot Module Support',
			'dir' => 'os/res',
			'files' => [
				'ModuleBase.h',
				'ModuleBase.cc',
			],
		},
		{
			'vname' => 'C interface; headers and glue code',
			'name' => 'os_krn_cface',
			'dir' => 'os/krn/cface',
			'comp' => [
				{
					'vname' => 'Common parts of the OctoPOS C Interface',
					'name' => 'os_krn_cface_common',
					'depends' => '&os_krn_cface_common',
					'files' => [
						'octo_types.h',
						'octo_leon.h',
						'octo_guest.h',
						'octo_guest.cc',
						'octo_cas.h',
						'octo_cas.cc',
						'header/octo_l2c.h',
						'mod/octo_l2c.cc',
						'octo_ldma.h',
						'octo_ldma.cc',
						'header/octo_app.h',
						'header/octo_cilk_support.h',
						'header/octo_claim.h',
						'header/octo_dispatch_claim.h',
						'header/octo_dispatch_claim_types.cc.opq',
						'header/octo_fls.h',
						'header/octo_hw_info.h',
						'header/octo_sharq.h',
						'header/octo_ilet.h',
						'header/octo_ilet_types.cc.opq',
						'header/octo_os_info.h',
						'header/octo_proxy_claim.h',
						'header/octo_proxy_claim_types.cc.opq',
						'header/octo_spinlock.h',
						'header/octo_spinlock_types.cc.opq',
						'header/octo_sys_memory.h',
						'header/octo_syscall_future.h',
						'header/octo_syscall_future_types.cc.opq',
						'mod/octo_app.cc',
						'mod/octo_cilk_support.cc',
						'mod/octo_claim.cc',
						'mod/octo_dispatch_claim.cc',
						'mod/octo_hw_info.cc',
						'mod/octo_sharq.cc',
						'mod/octo_ilet.cc',
						'mod/octo_fls.cc',
						'mod/octo_libc_alloc.cc',
						'mod/octo_os_info.cc',
						'mod/octo_proxy_claim.cc',
						'mod/octo_spinlock.cc',
						'mod/octo_syscall_future.cc',
						'header/octo_sem.h',
						'header/octo_sem_types.cc.opq',
						'mod/octo_sem.cc',
					],
				},
				{
					'vname' => 'Agents - C interface',
					'name' => 'os_krn_cface_agent',
					'depends' => '&os_krn_cface_common && &os_agent_support && not &os_agent2_support',
					'comp' => [
						{
							'srcfile' => 'octo_agent.cc',
							'file' => 'octo_agent.cc',
						},
						{
							'srcfile' => 'octo_agent.h',
							'file' => 'octo_agent.h',
						},
					],
				},
				{
					'vname' => 'Agents - C interface',
					'name' => 'os_krn_cface_agent2',
					'depends' => '&os_krn_cface_common && &os_agent2_support && not &os_agent_support',
					'comp' => [
						{
							'srcfile' => 'octo_agent2.cc',
							'file' => 'octo_agent.cc',
						},
						{
							'srcfile' => 'octo_agent2.h',
							'file' => 'octo_agent.h',
						},
						{
							'srcfile' => 'octo_agent_dummy.h',
							'file' => 'octo_agent2.h',
						},
					],
				},
				{
					'vname' => 'Agents - C interface',
					'name' => 'os_krn_cface_agent_both',
					'depends' => '&os_krn_cface_common && &os_agent2_support && &os_agent_support',
					'comp' => [
						{
							'srcfile' => 'octo_agent2.cc',
							'file' => 'octo_agent2.cc',
						},
						{
							'srcfile' => 'octo_agent2.h',
							'file' => 'octo_agent2.h',
						},
						{
							'srcfile' => 'octo_agent.cc',
							'file' => 'octo_agent.cc',
						},
						{
							'srcfile' => 'octo_agent.h',
							'file' => 'octo_agent.h',
						},
					],
				},
				{
					'vname' => 'VGA-C-Interface',
					'name' => 'os_krn_cface_vga',
					'depends' => '&os_krn_cface_common && &os_dev_vga',
					'files' => [
						'header/octo_vga.h',
						'mod/vga.cc',
					],
				},
				{
					'vname' => 'For LEON with APBUART',
					'name' => 'os_krn_cface_leon',
					'depends' => '&os_krn_cface_leon',
					'files' => [
						'octopos_interface_leon.cc',
					],
				},
				{
					'vname' => 'For x86guest',
					'name' => 'os_krn_cface_x86guest',
					'depends' => '&os_krn_cface_x86guest',
					'files' => [
						'octopos_interface_x86guest.cc',
					],
				},
				{
					'vname' => 'For x64native',
					'name' => 'os_krn_cface_x64native',
					'depends' => '&os_krn_cface_x64native',
					'files' => [
						'octopos_interface_x64native.cc',
					],
				},
				{
					'vname' => 'Debug stuff for applications',
					'name' => 'os_krn_cface_debug_functions',
					'depends' => '&os_krn_cface_common',
					'files' => [
						'header/octo_debug.h',
						'mod/octo_debug.cc',
					],
				},
				{
					'vname' => 'Event interface',
					'name' => 'os_krn_cface_event',
					'depends' => '&os_krn_cface_common && &os_res_event',
					'files' => [
						'header/octo_event.h',
						'mod/octo_event.cc',
					],
					'create_symlink' => [
						'../../../res/octo_event.h',
						'header/octo_event_types.h',
					],
				},
				{
					'vname' => 'Pull-DMA interface',
					'name' => 'os_krn_cface_pullDMA',
					'depends' => '&os_krn_cface_common',
					'files' => [
						'header/octo_pull_dma.h',
						'mod/octo_pull_dma.cc',
					]
				},
				{
					'vname' => 'TCPA interface',
					'name' => 'os_krn_cface_TCPAProxyClaim',
					'depends' => '&os_krn_cface_common && &os_res_tcpaProxyClaim',
					'files' => [
						'header/octo_tcpa.h',
						'header/octo_tcpa_types.cc.opq',
						'mod/octo_tcpa.cc',
					]
				},
				{
					'vname' => 'iLet id & application class interface',
					'name' => 'os_krn_cface_ilet_id_applClass',
					'depends' => '&os_krn_cface_common && &os_proc_ilet_applClass',
					'files' => [
						'header/octo_ilet_appclass_id.h',
						'mod/octo_ilet_appclass_id.cc',
					],
				},
				{
					'vname' => 'IP interface',
					'name' => 'os_krn_cface_ip',
					'depends' => '&platform_has_iotile && &os_net_ip',
					'files' => [
						'header/octo_ip.h',
						'mod/octo_ip.cc',
					],
				},
				{
					'vname' => 'TCP interface',
					'name' => 'os_krn_cface_tcp',
					'depends' => '&platform_has_iotile && &os_net_tcp',
					'files' => [
						'header/octo_tcp.h',
						'mod/octo_tcp.cc',
					],
				},
				{
					'vname' => 'Support clock() function',
					'name' => 'os_krn_cface_clock',
					'depends' => '&os_krn_cface_common && &os_dev_wallClock',
					'files' => [
						'header/octo_clock.h',
						'mod/octo_clock.cc',
					],
				},
				{
					'vname' => 'Accessor functions to tile properties and general utility functions',
					'name' => 'os_krn_cface_tile_props',
					'depends' => '&os_krn_cface_common',
					'files' => [
						'header/octo_tile.h',
						'mod/octo_tile.cc',
					]
				},
				{
					'vname' => 'C interface for ethernet communication channels',
					'name' => 'os_krn_cface_eth',
					'depends' => '&os_krn_cface_common && &platform_has_iotile',
					'files' => [
						'header/octo_eth.h',
						'mod/octo_eth.cc',
					],
				},
				{
					'vname' => 'Signal data types for synchronisation between iLets',
					'name' => 'os_krn_cface_signal',
					'depends' => '&os_krn_cface_common && &os_ipc_infectSignal &&
									(&os_ipc_simpleSignalDWCAS || &os_ipc_simpleSignalCAS)',
					'files' => [
						'header/octo_signal.h',
						'header/octo_signal_types.cc.opq',
						'mod/octo_signal.cc',
					],
				},
				{
					'vname' => 'Timer functions',
					'name' => 'os_krn_cface_timer',
					'depends' => '&os_krn_cface_common && &os_dev_stopwatch',
					'files' => [
						'header/octo_timer.h',
						'mod/octo_timer.cc',
					],
				},
			],
		},
		{
			'vname' => 'Hardware Emulations',
			'name' => 'hw_dev_emulation',
			'subdir' => 'hw/dev',
			'comp' => [
				{
					'vname' => 'CiC Emulator Driver Files',
					'name' => 'hw_dev_cic_emu',
					'depends' => '&hw_dev_cic_emu && ! &os_dev_offload',
					'files' => [
						'CiCEmulator.h',
						'CiCEmulator.cc',
					],
				},
				{
					'vname' => 'CiC Emulator Driver Files',
					'name' => 'hw_dev_cic_emu',
					'depends' => '&hw_dev_cic_emu && &os_dev_offload',
					'files' => [
						'CiCEmulator_Base.h',
						'CiCEmulator_Init.ah',
					],
					'comp' => [
						{
							'srcfile' => 'CiCEmulator_Offload.h',
							'file' => 'CiCEmulator.h',
						},
					],
				},
				{
					'vname' => 'Use CiC Emulator',
					'name' => 'os_dev_cic_emu',
					'depends' => '&os_dev_cic_emu',
					'dir' => 'hw/hal',
					'file' => 'CiC.h',
				},
				{
					'vname' => 'Stop-the-world functionality',
					'name' => 'hw_hal_worldstopper',
					'dir' => 'hw/hal',
					'file' => 'WorldStopper.h',
				},
				{
					'vname' => 'Hardware Information (Base)',
					'name' => 'hw_dev_hardware_information_base',
					'files' => [
						'HWInfo_Base.cc',
						'HWInfo_Base.h',
					],
				},
			],
		},
		{
			'vname' => 'Common HAL files',
			'name' => 'hw_hal_common',
			'dir' => 'hw/hal',
			'files' => [
				'AllocatorInit.h',
				'AtomicCommon.h',
				'CPULocalStorage.cc',
				'CPULocalStorage.h',
				'Compiler.h',
				'die.h',
				'GuardPage.h',
				'init.h',
				'IRQ.h',
				'IRQHandlers.cc',
				'MapFlags.h',
				'OSConstructors.cc',
				'OSConstructors.h',
				'PageMap.h',
				'MemoryType.cc',
			],
			'comp' => [
				{
					'subdir' => '!ARCH',
					'files' => [
						'die.cc'
					],
				},
				{
					'vname' => 'Stack guard page',
					'name' => 'hw_hal_guard_page',
					'depends' => '! &hw_hal_x86guest',
					'files' => [
						'GuardPage.cc',
					]
				},
			],
		},
		{
			'vname' => 'Devices',
			'name' => 'os_dev',
			'subdir' => 'os/dev',
			'comp' => [
				{
					'vname' => 'Basic files for device abstractions',
					'name' => 'os_dev_basis',
					'files' => [
						'DriverSingletons.ah',
						'HWInfo.cc',
						'HWInfo.h',
						'HWInfo_Inst.ah',
						'SHARQ.h',
						'iNoC.h',
						'iNoC.cc',
						'iNoC_Init.ah',
						'iNoC_Inst.ah',
						'iNoC_IRQBinding.ah',
						'IRQController.cc',
						'IRQController.h',
						'IRQController_Init.ah',
						'IRQController_Inst.ah',
						'Tile.cc',
						'Tile.h',
						'TLM.cc',
						'TLM.h',
						'SHM.cc',
						'SHM.h',
					],
				},
				{
					'vname' => 'AST',
					'name' => 'os_dev_ast',
					'depends' => '&os_dev_ast',
					'files' => [
						'AST.h',
						'AST.cc',
						'AST_IRQBinding.ah',
						'AST_Init.ah',
					],
				},
				{
					'vname' => 'TimerJob',
					'name' => 'os_dev_timer_job',
					'depends' => '&os_dev_timer_job',
					'files' => [
						'TimerJob.h',
						'TimerJob.cc',
						'TimerJob_Init.ah',
						'TimerJob_Inst.ah',
					],
				},
				{
					'vname' => 'Wall-clock device driver',
					'name' => 'os_dev_wallClock',
					'depends' => '&os_dev_wallClock',
					'files' => [
						'WallClock.h',
						'WallClock.cc',
						'WallClock_Init.ah',
						'WallClock_Inst.ah',
					],
				},
				{
					'vname' => 'UART0',
					'name' => 'os_dev_uart0',
					'depends' => '&os_dev_uart0',
					'files' => [
						'UART0.h',
						'UART0.cc',
						'UART0_Init.ah',
						'UART0_Inst.ah',
						'UART0_IRQBinding.ah',
					],
				},
				{
					'vname' => 'UART Blocking Op',
					'name' => 'os_dev_uart_blocking',
					'depends' => '&os_dev_uart_blocking',
					'files' => [
						'UART_Blocking.ah'
					],
				},
				{
					'vname' => 'CiC Device Driver',
					'name' => 'os_dev_cic',
					'depends' => '&os_dev_cic',
					'files' => [
						'CiC.h',
						'CiC.cc',
						'CiC_Init.ah',
						'CiC_Inst.ah',
						'CiC_IRQBinding.ah',
					],
				},
				{
					'vname' => 'Emulate synchronous ilet functions and DMA through sysilets',
					'name' => 'os_dev_inoc_emul',
					'depends' => '&os_dev_inoc_emul',
					'files' => [
						'iNoC_Emulate_iLetOps.ah',
						'iNoC_Emulate_iLetOps.cc',
					],
				},
				{
					'vname' => 'TCPA Device Driver',
					'name' => 'os_dev_tcpa',
					'depends' => '&os_dev_tcpa',
					'files' => [
						'TCPA.h',
						'TCPA_Init.ah',
						'TCPA.cc',
					],
				},
				{
					'vname' => 'TCPA Dummy Device Driver',
					'name' => 'os_dev_tcpa',
					'depends' => '! &os_dev_tcpa',
					'comp' => [
						{
							'srcfile' => 'TCPA_Dummy.h',
							'file' => 'TCPA.h',
						},
						{
							'file' => 'TCPA_Init.ah',
						},
					],
				},
				{
					'vname' => 'VGA Controller',
					'name' => 'os_dev_vga',
					'depends' => '&os_dev_vga',
					'files' => [
						'VGA.h',
						'VGA.cc',
						'VGA_Init.ah',
						'VGA_Inst.ah',
					],
				},
				{
					'vname' => 'Garbage-collector support',
					'name' => 'os_dev_gcsupport',
					'files' => [
						'WorldStopper.h',
						'WorldStopper.cc',
						'WorldStopper_Init.ah',
						'WorldStopper_IRQBinding.ah',
					],
				},
				{
					'vname' => 'Offloading CPU',
					'depends' => '&os_dev_offload',
					'name' => 'os_dev_offload',
					'files' => [
						'Offload.cc',
						'Offload.h',
						'Offload.ah',
					],
				},
				{
					'vname' => 'I/O tile common',
					'name' => 'os_dev_ioTile',
					'files' => [
						'IOTile.cc',
					],
					'comp' => [
						{
							'vname' => 'Real I/O tile',
							'name' => 'os_dev_ioTile_real',
							'depends' => '&platform_has_iotile',
							'files' => [
								'IOTile.h',
							],
						},
						{
							'vname' => 'Dummy I/O tile',
							'name' => 'os_dev_ioTile_dummy',
							'depends' => '! &platform_has_iotile',
							'srcfile' =>  'IOTile_Dummy.h',
							'file' =>  'IOTile.h',
						},
					],
				},
				{
					'vname' => 'Performance Counters',
					'name' => 'os_dev_perfmon',
					'depends' => '&os_dev_perfmon',
					'files' => [
						'PerfMon.h',
						'PerfMon.cc',
						'PerfMon_EventBinding.ah',
						'PerfMon_Slice.ah',
					],
				},
				{
					'vname' => 'Stopwatch for time measurements',
					'name' => 'os_dev_stopwatch',
					'depends' => '&os_dev_stopwatch',
					'files' => [
						'Stopwatch.h',
					],
				},
			],
		},
		{
			'vname' => 'IRQ',
			'name' => 'os_irq',
			'subdir' => 'os/irq',
			'comp' => [
				{
					'vname' => 'Deferred Fuction Calls',
					'name' => 'os_irq_dfcGate',
					'depends' => '&os_irq_dfcGate',
					'files' => [
						'DFCGate.h',
						'DFCExecutor.ah',
						'DFCExecutor_Slice.ah',
					],
				},
				{
					'vname' => 'Guard',
					'name' => 'os_irq_guard',
					'depends' => '&os_irq_guard_proepilog || &os_irq_guard_hardsimple',
					'files' => [
						'Guard.h',
						'Guard.cc',
						'IRQ_Sync.ah',
					],
				},
				{
					'vname' => 'Guard Pro/Epilog',
					'name' => 'os_irq_guard_proepilog',
					'depends' => '&os_irq_guard_proepilog',
					'files' => [
						'Guard_PrologEpilog.ah',
						'Guard_PrologEpilog_Slice.ah',
						'IRQExecutor_ProEpi.ah',
						'IRQExecutor_ProEpi_Slice.ah',
					],
				},
				{
					'vname' => 'Guard HardSync',
					'name' => 'os_irq_guard_hardsimple',
					'depends' => '&os_irq_guard_hardsimple',
					'files' => [
						'Guard_HardSync.ah',
						'Guard_HardSync.cc',
						'IRQExecutor_Hard.ah',
					],
				},
				{
					'vname' => 'IPI Executor',
					'name' => 'os_irq_ipi_executor',
					'files' => [
						'IPIExecutor.h',
						'IPIExecutor.cc',
						'IPIExecutor_IRQBinding.ah',
					],
				},
			],
		},
		{
			vname => 'Control-Flow Abstractions',
			name => 'os_proc',
			subdir => 'os/proc',
			comp => [
				{
					'vname' => 'Basic OctoPOS stuff',
					'name' => 'os_proc_basis',
					'files' => [
						'AppInfo.h',
						'AppInfo.cc',
						'Application.h',
						'Application.cc',
						'ApplicationClass.h',
						'AppRegion.h',
						'AppRegion.cc',
						'Claim.cc',
						'Claim.h',
						'Claim_Init.ah',
						'ClaimTag.h',
						'Context.cc',
						'Context.h',
						'ContextManager.cc',
						'ContextManager.h',
						'ContextManager_Init.ah',
						'ExecutionManager.h',
						'Executor.h',
						'FibreLocalStorage.cc',
						'FibreLocalStorage.h',
						'iLet.cc',
						'iLet.h',
						'InvasiveScheduler.cc',
						'InvasiveScheduler.h',
						'WorkStealingScheduler.cc',
						'WorkStealingScheduler.h',
					],
				},
				{
					'vname' => 'Add call to print blocked Contextes to ContextManager',
					'name' => 'cf_os_proc_context_print_blocked',
					'depends' => '&cf_os_proc_context_print_blocked',
				},
				{
					'vname' => 'Add sanity checks to context handling',
					'name' => 'os_proc_context_sanity_checks',
					'depends' => '&os_proc_context_sanity_checks',
					'files' => [
						'ContextManager_SanityChecks.ah',
						'ContextManager_SanityChecks_Slice.ah',
					],
				},
				{
					'vname' => 'Context Stealing with ABP Queue',
					'name' => 'os_proc_contextManagerQueueAbp',
					'depends' => '&os_proc_contextManagerQueueAbp',
					'srcfile' => 'ContextManagerQueue_Abp.h',
					'file' => 'ContextManagerQueue.h',
				},
				{
					'vname' => 'Context Stealing with CL Queue',
					'name' => 'os_proc_contextManagerQueueCl',
					'depends' => '&os_proc_contextManagerQueueCl',
					'srcfile' => 'ContextManagerQueue_Cl.h',
					'file' => 'ContextManagerQueue.h',
				},
				{
					'vname' => 'Support application classes in iLet',
					'name' => 'os_proc_ilet_applClass',
					'depends' => '&os_proc_ilet_applClass',
					'files' => [
						'ILet_CiC_ApplClass.ah',
						'ILet_CiC_ApplClass_Slice.ah',
					],
				},
				{
					'vname' => 'Add i-let function checks to dispatcher',
					'name' => 'os_proc_dispatcher_iLetFuncCheck',
					'depends' => '&os_proc_dispatcher_iLetFuncCheck',
					'files' => [
						'Dispatcher_iLetFuncCheck.ah',
					],
				},
				{
					'vname' => 'Count i-lets and sys-i-lets to see if something gets lost',
					'name' => 'os_proc_iLetCounter',
					'depends' => '&os_proc_iLetCounter',
					'files' => [
						'iLetCounter.cc',
						'iLetCounter.h',
						'iLetCounter_Gather.ah',
					],
				},
				{
					'vname' => 'Print i-let and sys-i-let counters at shutdown',
					'name' => 'os_proc_iLetCounterPrint',
					'depends' => '&os_proc_iLetCounterPrint',
					'files' => [
						'iLetCounter_Print.ah',
					],
				},
				{
					'vname' => 'Print Stack Usage',
					'name' => 'os_proc_StackUsage',
					'depends' => '&os_proc_StackUsage',
					'files' => [
						'StackUsage.ah',
						'StackUsage.cc',
						'StackUsage_Slice.ah',
					],
				},
			],
		},
		{
			'vname' => 'Resource Allocation and Accounting',
			'name' => 'os_res',
			'subdir' => 'os/res',
			'comp' => [
				{
					'vname' => 'Event configuration',
					'name' => 'os_res_event',
					'depends' => '&os_res_event',
					'files' => [
						'Event.h',
						'Event.cc',
						'octo_event.h',
						'Event_Meta.ah',
					],
					'comp' => [
						{
							'vname' => 'Time via WallClock',
							'name' => 'os_res_event_time_wallclock',
							'depends' => '&os_dev_wallClock',
							'file' => 'Event_Time_Wallclock.ah',
						},
					],
				},
				{
					'vname' => 'Compress event transports',
					'name' => 'os_res_event_transport_compress',
					'depends' => '&os_res_event_transport_compress',
					'file' => 'Event_Transport_Compress.ah',
				},
				{
					'vname' => 'Basic OctoPOS stuff for resource management',
					'name' => 'os_res_basis',
					'files' => [
						'AtomicBitmapAllocator.h',
						'BlockAllocatorCommon.h',
						'BlockBumpAllocator.h',
						'BlockListAllocator.h',
						'DispatchClaim.cc',
						'DispatchClaim.h',
						'GCAllocator.cc',
						'GCAllocator.h',
						'KernelAllocators.cc',
						'ListAllocator.h',
						'ListBlockAllocator.h',
						'ProxyClaim.cc',
						'ProxyClaim.h',
						'ProxyClaim_Init.ah',
						'ResourceManager.cc',
						'ResourceManager.h',
						'ResourceManager_Init.ah',
						'TreeBlockAllocator.h',
						'WrapperAllocator.h',
					],
				},
				{
					'vname' => 'TLM kernel allocator using RBTrees',
					'name' => 'os_res_tlm_kernel_tree_alloc',
					'depends' => '&os_res_tlm_kernel_tree_alloc',
					'srcfile' => 'TreeTLMKernelAllocator.h',
					'file' => 'TLMKernelAllocator.h',
				},
				{
					'vname' => 'ICM kernel allocator using RBTrees',
					'name' => 'os_res_icm_kernel_tree_alloc',
					'depends' => '&os_res_icm_kernel_tree_alloc',
					'srcfile' => 'TreeICMKernelAllocator.h',
					'file' => 'ICMKernelAllocator.h',
				},
				{
					'vname' => 'SHM kernel allocator using RBTrees',
					'name' => 'os_res_shm_kernel_tree_alloc',
					'depends' => '&os_res_shm_kernel_tree_alloc',
					'srcfile' => 'TreeSHMKernelAllocator.h',
					'file' => 'SHMKernelAllocator.h',
				},
				{
					'vname' => 'TLM kernel allocator using lists',
					'name' => 'os_res_tlm_kernel_list_alloc',
					'depends' => '&os_res_tlm_kernel_list_alloc',
					'srcfile' => 'ListTLMKernelAllocator.h',
					'file' => 'TLMKernelAllocator.h',
				},
				{
					'vname' => 'ICM kernel allocator using lists',
					'name' => 'os_res_icm_kernel_list_alloc',
					'depends' => '&os_res_icm_kernel_list_alloc',
					'srcfile' => 'ListICMKernelAllocator.h',
					'file' => 'ICMKernelAllocator.h',
				},
				{
					'vname' => 'SHM kernel allocator using lists',
					'name' => 'os_res_shm_kernel_list_alloc',
					'depends' => '&os_res_shm_kernel_list_alloc',
					'srcfile' => 'ListSHMKernelAllocator.h',
					'file' => 'SHMKernelAllocator.h',
				},
				{
					'vname' => 'TCPAProxyClaim: Interface for allocating and using TCPA resources',
					'name' => 'os_res_tcpaProxyClaim',
					'depends' => '&os_res_tcpaProxyClaim',
					'files' => [
						'TCPAProxyClaim.h',
						'TCPAProxyClaim.cc',
					],
				},
				{
					'vname' => 'TCPAProxyClaim RPC server side',
					'name' => 'os_res_tcpaProxyClaim_RPC_server',
					'depends' => '&os_res_tcpaProxyClaim_RPC_server',
					'files' => [
						'TCPAProxyClaimRPCServer.cc',
					],
				},
				{
					'vname' => 'TCPAProxyClaim common RPC headers',
					'name' => 'os_res_tcpaProxyClaim_RPC_header',
					'depends' => '&os_res_tcpaProxyClaim || &os_res_tcpaProxyClaim_RPC_server',
					'files' => [
						'TCPAProxyClaimRPCHeader.h',
					],
				},
				{
					'vname' => 'SystemClaim Abstration',
					'depends' => '&cf_os_res_systemClaim || &os_dev_offload',
					'name' => 'os_dev_offload',
					'files' => [
						'SystemClaim.cc',
						'SystemClaim.h',
					],
				},
				{
					'vname' => 'SystemClaim Dummy Abstration',
					'depends' => '! (&cf_os_res_systemClaim || &os_dev_offload)',
					'name' => 'os_dev_offload_dummy',
					'comp' => [
						{
							'srcfile' => 'SystemClaim_Dummy.h',
							'file' => 'SystemClaim.h',
						},
						{
							'srcfile' => 'SystemClaim_Dummy.cc',
							'file' => 'SystemClaim.cc',
						},
					],
				},
				{
					'vname' => 'Some profiling stuff',
					'name' => 'os_res_profiling',
					'depends' => '&cf_os_res_profiling',
					'files' => [
						'Claim_Time_Slice.ah',
						'Claim_Time.ah',
						'iLetHistogram.h',
						'iLetHistogram.cc',
						'iLetHistogram_Init.ah',
						'DmaHistogram.h',
						'DmaHistogram.cc',
						'DmaHistogram_Init.ah',
					],
				},
			],
		},
		{
			'vname' => 'Agent System',
			'name' => 'os_agent_support',
			'depends' => '&os_agent_support',
			'subdir' => 'os/agent',
			'comp' => [
				{
					'vname' => 'Actual Agents Implementation',
					'name' => 'os_agent_impl',
					'depends' => '&os_agent_impl',
					'files' => [
						'AbstractAgentOctoClaim.h',
						'AbstractAgentOctoClaim.cc',
						'AbstractConstraint.cc',
						'AbstractConstraint.h',
						'ActorConstraint.cc',
						'ActorConstraint.h',
						'ActorConstraintSolver.cc',
						'ActorConstraintSolver.h',
						'Agent.cc',
						'Agent.h',
						'Agent_Init.ah',
						'AgentClaim.h',
						'AgentClaim.cc',
						'AgentConstraint.cc',
						'AgentConstraint.h',
						'AgentMemory.h',
						'AgentRPCClient.h',
						'AgentRPCHeader.h',
						'AgentRPCImpl.h',
						'AgentSystem.cc',
						'AgentSystem.h',
						'Cluster.cc',
						'Cluster.h',
						'ClusterGuarantee.cc',
						'ClusterGuarantee.h',
						'FlatCluster.cc',
						'FlatCluster.h',
						'FlatClusterGuarantee.cc',
						'FlatClusterGuarantee.h',
						'FlatOperatingPoint.cc',
						'FlatOperatingPoint.h',
						'OperatingPoint.cc',
						'OperatingPoint.h',
						'ProxyAgentOctoClaim.h',
						'ProxyAgentOctoClaim.cc',
						'Platform.h',
						'Platform.cc',
					],
				},
				{
					'vname' => 'Agents on I/O tile (direct calls)',
					'name' => 'os_agent_rpc_io',
					'depends' => '&os_agent_rpc_io && &os_agent_impl',
					'srcfile' => 'AgentRPCClient_IO.cc',
					'file' => 'AgentRPCClient.cc',
				},
				{
					'vname' => 'RPC implementation',
					'name' => 'os_agent_rpc_impl',
					'depends' => '&os_agent_rpc_io && &os_agent_impl',
					'file' => 'AgentRPCServer.cc',
				},
				{
					'vname' => 'Agents on non-I/O tile (RPCs)',
					'name' => 'os_agent_rpc_nonio',
					'depends' => '&os_agent_rpc_nonio && &os_agent_impl',
					'srcfile' => 'AgentRPCClient_NonIO.cc',
					'file' => 'AgentRPCClient.cc',
				},
				{
					'vname' => 'Heterogeneous Agents implementation',
					'name' => 'os_agent_rpc_hetero',
					'depends' => '&os_agent_rpc_hetero && &os_agent_impl',
					'srcfile' => 'AgentRPCClient_Hetero.cc',
					'file' => 'AgentRPCClient.cc',
				},
				{
					'vname' => 'Agent System Metrics',
					'name' => 'os_agent_metrics',
					'depends' => '&cf_gui_enabled && &platform_has_iotile',
					'files' => [
						'Metric.cc',
						'Metric.h',
						'MetricAgentInvade.cc',
						'MetricAgentInvade.h',
						'MetricAgentRetreat.cc',
						'MetricAgentRetreat.h',
						'MetricDeletedAgent.cc',
						'MetricDeletedAgent.h',
						'MetricNewAgent.cc',
						'MetricNewAgent.h',
						'MetricSender.cc',
						'MetricSender.h',
						'MetricSystemArchitecture.cc',
						'MetricSystemArchitecture.h',
						'MetricAgentRename.cc',
						'MetricAgentRename.h',
					],
				},
			],
		},
		{
			'vname' => 'Agent System 2.0',
			'name' => 'os_agent2_support',
			'depends' => '&os_agent2_support',
			'subdir' => 'os/agent2',
			'comp' => [
				{
					'vname' => 'Agent System 2.0 Communication',
					'name' => 'os_agent2_impl',
					'depends' => '&os_agent2_support',
					'files' => [
						'agent2_lib.h',
						'common.h',
						'config.h',
						'ActorConstraint.cc',
						'ActorConstraint.h',
						'Agent2.cc',
						'Agent2.h',
						'Agent2_Init.ah',
						'AgentClaim.cc',
						'AgentClaim.h',
						'AgentClusterManager.cc',
						'AgentClusterManager.h',
						'AgentClusterManagerRPC.h',
						'AgentConstraint.cc',
						'AgentConstraint.h',
						'AgentInterface.cc',
						'AgentInterface.h',
						'AgentID.h',
						'AgentID.cc',
						'AgentMemory.cc',
						'AgentMemory.h',
						'AgentRPC.h',
						'AgentSerialization.h',
						'AgentTileManager.cc',
						'AgentTileManager.h',
						'AgentTileManagerRPC.h',
						'CI.cc',
						'CI.h',
						'Cluster.cc',
						'Cluster.h',
						'ClusterGuarantee.cc',
						'ClusterGuarantee.h',
						'CommInterface.h',
						'Constraint.cc',
						'Constraint.h',
						'HashMap.h',
						'OperatingPoint.cc',
						'OperatingPoint.h',
						'AgentLRUCache.h',				
						
					],
				},
				{
                    'vname' => 'Agent System 2.0 Custom Metrics',
					'name' => 'os_agent2_metrics_impl',
					'depends' => '&os_agent2_support && &cf_agent2_metrics_custom',
					'files' => [
                        'AgentMetrics.h',
						'AgentMetrics.cc',
						'AgentMetricsPrinter.h',
						'AgentMetricsPrinter.cc',
						'MetricsEnum.h'
					]
				},
				{
					'vname' => 'LibKit Header',
					'name' => 'os_agent2_libkit_headers',
					'depends' => '&os_agent2_support',
					'subdir' => 'libkit',
					'files' => [
						'include/LfHashMap.h',
						'include/LfList.h',
						'include/LfListIterator.h',
						'include/LfTypes.h',
						'include/libkitconfig.h',
					],
				},
				{
					'vname' => 'Agent System Metrics',
					'name' => 'os_agent2_metrics',
					'depends' => '&cf_gui_enabled && &platform_has_iotile',
					'files' => [
						'Metric.cc',
						'Metric.h',
						'MetricAgentInvade.cc',
						'MetricAgentInvade.h',
						'MetricAgentRename.cc',
						'MetricAgentRename.h',
						'MetricAgentRetreat.cc',
						'MetricAgentRetreat.h',
						'MetricDeletedAgent.cc',
						'MetricDeletedAgent.h',
						'MetricNewAgent.cc',
						'MetricNewAgent.h',
						'MetricSender.cc',
						'MetricSender.h',
						'MetricSystemArchitecture.cc',
						'MetricSystemArchitecture.h',
					],
				},
			],
		},
		{
			'vname' => 'IPC Facilities',
			'name' => 'os_ipc',
			'subdir' => 'os/ipc',
			'comp' => [
				{
					'vname' => 'Basic files for synchronisation between i-lets',
					'name' => 'os_ipc_basis',
					'files' => [
						'BinarySignal.h',
						'BinarySignalInternal.h',
						'BinarySignalInternal.cc',
						'Semaphore.h',
						'SemaphoreBlockingImpl.h',
						'SemaphoreBlockingImpl.cc',
						'SignalBase.h',
						'SyscallFuture.h',
					],
				},
				{
					'vname' => 'Simple Signal Base Class',
					'name' => 'os_ipc_simpleSignalBase',
					'depends' => '&os_ipc_simpleSignalDWCAS || &os_ipc_simpleSignalCAS',
					'files' => [
						'SimpleSignalBase.h',
					],
				},
				{
					'vname' => 'SimpleSignal implemented with DWCAS',
					'name' => 'os_ipc_simpleSignalDWCAS',
					'depends' => '&os_ipc_simpleSignalDWCAS',
					'files' => [
						'SimpleSignalDWCAS.h'
					],

				},
				{
					'vname' => 'SimpleSignal implemented with CAS',
					'name' => 'os_ipc_simpleSignalCAS',
					'depends' => '&os_ipc_simpleSignalCAS',
					'files' => [
						'SimpleSignalCAS.h'
					],

				},
				{
					'vname' => 'Use CAS-implementation for SimpleSignal',
					'name' => 'os_ipc_simpleSignal_iface_cas',
					'depends' => '&os_ipc_simpleSignal_iface_cas',
					'srcfile' => 'SimpleSignalIface_CAS.h',
					'file' => 'SimpleSignal.h',
				},
				{
					'vname' => 'Use DWCAS-implementation for SimpleSignal',
					'name' => 'os_ipc_simpleSignal_iface_dwcas',
					'depends' => '&os_ipc_simpleSignal_iface_dwcas',
					'srcfile' => 'SimpleSignalIface_DWCAS.h',
					'file' => 'SimpleSignal.h',
				},
				{
					'vname' => 'Use wait-free implementation for SimpleSignal',
					'name' => 'os_ipc_simpleSignal_iface_waitfree',
					'depends' => '&os_ipc_simpleSignal_iface_waitfree',
					'srcfile' => 'SimpleSignalIface_Waitfree.h',
					'file' => 'SimpleSignal.h',
				},
				{
					'vname' => 'Infect Signal',
					'name' => 'os_ipc_infectSignal',
					'depends' => '&os_ipc_infectSignal',
					'files' => [
						'InfectSignal.h',
					]
				},
				{
					'vname' => 'Spin Signal',
					'name' => 'os_ipc_spinSignal',
					'files' => [
						'SpinSignal.h',
					]
				},
				{
					'vname' => 'Wait-free Signal',
					'name' => 'os_ipc_signal',
					'depends' => '&os_ipc_signal',
					'files' => [
						'Signal.h',
					]
				},
			],
		},
		{
			'vname' => 'RPC Facilities',
			'name' => 'os_rpc',
			'subdir' => 'os/rpc',
			'comp' => [
				{
					'vname' => 'Basic files for the RPC service',
					'name' => 'os_rpc_basis',
					'files' => [
						'Future.h',
						'OctoPOS_RPC.ah',
						'RPCAnswerEnvelope.h',
						'RPCService.ah',
						'RPCService_Slices.ah',
						'RPCStub.h',
						'RPCUtil.h',
						'SysILet.cc',
						'SysILet.h',
					],
				},
				{
					'vname' => 'SysILet CRC check',
					'name' => 'os_rpc_sysILet_crc_check',
					'depends' => '&os_rpc_sysILet_crc_check',
					'files' => [
						'SysILet_CRC_Check.ah',
						'SysILet_CRC_Check_Slice.ah',
					],
				},
			],
		},
		{
			'vname' => 'Low-level tests',
			'name' => 'lltest',
			'subdir' => 'lltest',
			'comp' => [
				{
					'vname' => 'iNoC reservation test and development',
					'name' => 'lltest_inoc_reservation',
					'depends' => '&lltest_inoc_reservation',
					'subdir' => 'inoc_reservation',
					'files' => [
						'inoc_reservation.cc',
					],
				},
				{
					'vname' => 'HW-CiC generic test/dev application',
					'name' => 'lltest_cic_dev',
					'depends' => '&lltest_cic_dev',
					'subdir' => 'cic_dev',
					'files' => [
						'cic_dev.cc',
					],
				},
				{
					'vname' => 'Testcase for possibly concurrent accesses to the DDR memory',
					'name' => 'lltest_ddr_test',
					'depends' => '&lltest_ddr_test',
					'subdir' => 'ddr_test',
					'files' => [
						'ddr_test.cc',
					],
				},
				{
					'vname' => 'Test iNoC DMA operations',
					'name' => 'lltest_dma_test',
					'depends' => '&lltest_dma_test',
					'subdir' => 'dma_test',
					'files' => [
						'dma_test.cc',
					],
				},
				{
					'vname' => 'Sysilet crosstest',
					'name' => 'lltest_sysilet_crosstest',
					'depends' => '&lltest_sysilet_crosstest',
					'subdir' => 'sysilet_crosstest',
					'files' => [
						'sysilet_dbg.cc',
						'sysilet_test.ah',
					],
				},
				{
					'vname' => 'Testcase for possibly concurrent accesses to the DDR memory',
					'name' => 'lltest_fsb_deadlock',
					'depends' => '&lltest_fsb_deadlock',
					'subdir' => 'fsb_deadlock',
					'files' => [
						'fsb_deadlock.cc',
					],
				},
				{
					'vname' => 'Measure stuff',
					'name' => 'lltest_measure_stuff',
					'depends' => '&lltest_measure_stuff',
					'subdir' => 'measure_stuff',
					'files' => [
						'measure_stuff.cc',
					],
				},
				{
					'vname' => 'Simple test for correct bootup',
					'name' => 'lltest_bootup',
					'depends' => '&lltest_bootup',
					'subdir' => 'bootup',
					'files' => [
						'bootup.cc',
					],
				},
				{
					'vname' => 'Simple test for invade core id',
					'name' => 'lltest_invade_coreid',
					'depends' => '&lltest_invade_coreid',
					'subdir' => 'invade_coreId',
					'files' => [
						'invade_coreId.cc',
					],
				},
				{
					'vname' => 'Demo for Begehung',
					'name' => 'lltest_demo',
					'depends' => '&lltest_demo',
					'subdir' => 'demo',
					'files' => [
						'demo.cc',
					],

				},
				{
					'vname' => 'Testing dma src/dst iLets and normal infect iLets',
					'name' => 'lltest_dma_ilet_test',
					'depends' => '&lltest_dma_ilet_test',
					'subdir' => 'dma_ilet_test',
					'files' => [
						'dma_ilet_test.cc',
					],
				},
				{
					'vname' => 'Test core-grained scheduling',
					'name' => 'lltest_coregrained_test',
					'depends' => '&lltest_coregrained_test',
					'subdir' => 'coregrained_test',
					'files' => [
						'coregrained_test.cc',
					]
				},
				{
					'vname' => 'Test correct eviction of cache lines in L2 cache',
					'name' => 'lltest_l2cache_eviction',
					'depends' => '&lltest_l2cache_eviction',
					'subdir' => 'l2cache_eviction',
					'files' => [
						'l2cache_eviction.cc',
					],
				},
				{
					'vname' => 'Simple MPI-Test',
					'name' => 'lltest_mpi_simple',
					'depends' => '&lltest_mpi_simple',
					'subdir' => 'mpi_simple',
					'files' => [
						'mpi_simple.cc',
					],
				},
				{
					'vname' => 'Client Server MPI-Test',
					'name' => 'lltest_mpi_client_server',
					'depends' => '&lltest_mpi_client_server',
					'subdir' => 'mpi_client_server',
					'files' => [
						'mpi_client_server.cc',
					],
				},
				{
					'vname' => 'Broadcast MPI-Test',
					'name' => 'lltest_mpi_broadcast',
					'depends' => '&lltest_mpi_broadcast',
					'subdir' => 'mpi_broadcast',
					'files' => [
						'mpi_broadcast.cc',
					],
				},
				{
					'vname' => 'Asynchronous MPI-Test',
					'name' => 'lltest_mpi_non_blocking',
					'depends' => '&lltest_mpi_non_blocking',
					'subdir' => 'mpi_non_blocking',
					'files' => [
						'mpi_non_blocking.cc',
					],
				},
				{
					'vname' => 'Reduce MPI-Test',
					'name' => 'lltest_mpi_reduce',
					'depends' => '&lltest_mpi_reduce',
					'subdir' => 'mpi_reduce',
					'files' => [
						'mpi_reduce.cc',
					],
				},
				{
					'vname' => 'NPB IS MPI-Test',
					'name' => 'lltest_mpi_IS',
					'depends' => '&lltest_mpi_IS',
					'subdir' => 'mpi_IS',
					'files' => [
						'mpi_IS.cc',
						'npbparams.h',
					],
				},
				{
					'vname' => 'Barrier MPI-Test',
					'name' => 'lltest_mpi_barrier',
					'depends' => '&lltest_mpi_barrier',
					'subdir' => 'mpi_barrier',
					'files' => [
						'mpi_barrier.cc',
					],
				},
				{
					'vname' => 'Split MPI-Test',
					'name' => 'lltest_mpi_split',
					'depends' => '&lltest_mpi_split',
					'subdir' => 'mpi_split',
					'files' => [
						'mpi_split.cc',
					],
				},
			],
		},
	],
};
