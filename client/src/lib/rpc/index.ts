import * as NPRPC from 'nprpc';
import * as npchat from './npchat';

export let poa: NPRPC.Poa;
// export let calculator: nscalc.Calculator;
// export let authorizator: nscalc.Authorizator;
// export let chat: nscalc.Chat;

export const init_rpc = async (): Promise<void> => {
	NPRPC.set_debug_level(NPRPC.DebugLevel.DebugLevel_Critical);
	let rpc = await NPRPC.init();
	poa = rpc.create_poa(10);
	// calculator = NPRPC.narrow(rpc.host_info.objects.calculator, nscalc.Calculator);
	// authorizator = NPRPC.narrow(rpc.host_info.objects.authorizator, nscalc.Authorizator);
	// chat = NPRPC.narrow(rpc.host_info.objects.chat, nscalc.Chat);
}