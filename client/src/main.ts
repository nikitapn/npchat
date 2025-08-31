import { mount } from 'svelte'
import './app.css'
import App from './App.svelte'

import { init_rpc } from './lib/rpc/index'
import { initI18n } from './lib/i18n'

let app: any;

// Initialize i18n and NPRPC first, then mount the app
Promise.all([initI18n(), init_rpc()]).then(() => {
  app = mount(App, {
    target: document.getElementById('app')!,
  })
}).catch(error => {
  console.error('Failed to initialize application:', error);
});

export default app
