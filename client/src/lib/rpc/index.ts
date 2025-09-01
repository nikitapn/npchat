import * as NPRPC from 'nprpc';
import * as npchat from './npchat';

export let poa: NPRPC.Poa;
export let authorizator: npchat.Authorizator;

export const init_rpc = async (): Promise<void> => {
  NPRPC.set_debug_level(NPRPC.DebugLevel.DebugLevel_TraceAll);
  let rpc = await NPRPC.init();
  poa = rpc.create_poa(10);
  authorizator = NPRPC.narrow(rpc.host_info.objects.authorizator, npchat.Authorizator);
}