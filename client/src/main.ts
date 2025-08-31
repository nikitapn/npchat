import { mount } from 'svelte'
import './app.css'
import App from './App.svelte'

import { init_rpc } from './lib/rpc/index'

init_rpc();

const app = mount(App, {
  target: document.getElementById('app')!,
})

export default app
