// Copyright (c) 2025 nikitapnn1@gmail.com

#include <exception>
#include <iostream>
#include <cassert>
#include <string>
#include <fstream>
#include <filesystem>

#include <spdlog/spdlog.h>

#include <nprpc/nprpc.hpp>
#include <nprpc/serialization/oarchive.h>

#include <boost/asio/signal_set.hpp>
#include <boost/beast/core/error.hpp>
#include <boost/program_options.hpp>

#include "services/boost/di.hpp"

#include "services/db/Database.hpp"
#include "services/db/AuthService.hpp"
#include "services/db/ContactService.hpp"
#include "services/db/MessageService.hpp"
#include "services/db/ChatService.hpp"

#include "services/rpc/Authorizator.hpp"

#include "util/util.hpp"
#include "util/HostJsonMacros.hpp"

DEFINE_HOST_JSON_STRUCT(authorizator)

int main(int argc, char *argv[]) {
  namespace di = boost::di;
  namespace po = boost::program_options;
  namespace fs = std::filesystem;

  HostJson host_json;
  std::string hostname, http_dir, data_dir, public_cert, private_key, dh_params;
  unsigned short port;
  bool log_trace = false;

  po::options_description desc("Allowed options");
  desc.add_options()
    ("help", "produce help message")
    ("http-dir", po::value<std::string>(&http_dir)->required(), "HTTP root directory")
    ("data-dir", po::value<std::string>(&data_dir)->required(), "Data root directory")
    ("hostname", po::value<std::string>(&hostname)->default_value(""), "Hostname")
    ("port", po::value<unsigned short>(&port)->default_value(8080), "Port to listen")
    ("public-cert", po::value<std::string>(&public_cert)->default_value(""), "Path to the public certificate")
    ("private-key", po::value<std::string>(&private_key)->default_value(""), "Path to the certificate private key")
    ("dh-params", po::value<std::string>(&dh_params)->default_value(""), "Path to Diffie-Hellman parameters")
    ("get-sha256", po::value<std::string>(), "Return SHA256 of the password")
    ("trace", po::bool_switch(&log_trace)->default_value(false), "Enable log trace");

  try {
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);
    if (vm.count("help")) {
      std::cout << desc << "\n";
      return 0;
    }
    po::notify(vm);
  } catch (std::exception& e) {
    std::cerr << "Exception during command line parsing: " << e.what() << std::endl;
    return -1;
  }

  if (log_trace) {
    spdlog::set_level(spdlog::level::trace);
  } else {
    spdlog::set_level(spdlog::level::info);
  }

  try {
    auto builder = nprpc::RpcBuilder();
    builder
      .set_debug_level(nprpc::DebugLevel::DebugLevel_TraceAll)
      .set_listen_http_port(port)
      .set_http_root_dir(http_dir)
      .set_hostname(hostname);

    if ((public_cert.empty() ^ private_key.empty()) == true)
      throw std::runtime_error("Certificate and private key paths must be provided when using SSL.");

    bool use_ssl = !public_cert.empty();
    if (use_ssl)
      builder.enable_ssl_server(public_cert, private_key, dh_params);

    auto rpc = builder.build(thread_pool::get_instance().ctx());
    auto data_path = fs::canonical(fs::path(data_dir));

    auto firstInjector = [&] () { return di::make_injector(
      di::bind<>().to(*rpc),
      di::bind<Database>().in(di::singleton).to<Database>(
        (data_path / "npchat.sqlite3").generic_string()
      )
    );};

    auto injector = firstInjector();

    auto authService = injector.create<std::shared_ptr<AuthService>>();
    auto contactService = injector.create<std::shared_ptr<ContactService>>();
    auto messageService = injector.create<std::shared_ptr<MessageService>>();
    auto chatService = injector.create<std::shared_ptr<ChatService>>();

    auto injector2 = di::make_injector(
      firstInjector(),
      di::bind<>().to(authService),
      di::bind<>().to(contactService),
      di::bind<>().to(messageService),
      di::bind<>().to(chatService)
    );

    // static poa
    auto poa = nprpc::PoaBuilder(rpc)
		  .with_max_objects(4)
		  .with_lifespan(nprpc::PoaPolicy::Lifespan::Persistent)
		  .build();

    auto authorizator = injector2.create<std::shared_ptr<AuthorizatorImpl>>();

    // Capture SIGINT and SIGTERM to perform a clean shutdown
    boost::asio::signal_set signals(thread_pool::get_instance().executor(), SIGINT, SIGTERM);
    signals.async_wait([&](boost::beast::error_code const&, int) {
      thread_pool::get_instance().stop();
    });

    // Forbid unsecured WebSocket connections when SSL is enabled
    const auto flags = use_ssl ? nprpc::ObjectActivationFlags::ALLOW_SSL_WEBSOCKET
      : nprpc::ObjectActivationFlags::ALLOW_WEBSOCKET;

    host_json.secured = use_ssl;
    ACTIVATE_HOST_OBJECT(host_json, poa, authorizator, flags);
    SAVE_HOST_JSON_TO_FILE(host_json, http_dir);

    thread_pool::get_instance().ctx().run();
    thread_pool::get_instance().wait();

    // rpc->destroy();

  } catch (std::exception& ex) {
    spdlog::critical("Exception occurred: {}", ex.what());
    thread_pool::get_instance().stop();
    return EXIT_FAILURE;
  }

  spdlog::info("npchat is shutting down gracefully");

  return EXIT_SUCCESS;
}