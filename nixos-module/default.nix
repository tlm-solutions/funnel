{pkgs, config, lib, ... }:
let
  cfg = config.TLMS.funnel;
in
{
  options.TLMS.funnel = with lib; {
    enable = mkOption {
      type = types.bool;
      default = false;
      description = "enabeling funnel or not";
    };
    GRPC = {
      host = mkOption {
        type = types.str;
        default = "127.0.0.1";
        description = "on which address the grpc server should bind to";
      };
      port = mkOption {
        type = types.port;
        default = 50052;
        description = "on which port the grpc server should bind to";
      };
    };
    defaultWebsocket = {
      host = mkOption {
        type = types.str;
        default = "127.0.0.1";
        description = "host of the websocket";
      };
      port = mkOption {
        type = types.port;
        default = 9002;
        description = "port of the websocket";
      };
    };
    metrics = {
      port = mkOption {
        type = types.port;
        default = 9003;
        description = "prometheus metrics port";
      };
      host = mkOption {
        type = types.str;
        default = "127.0.0.1";
        description = "address prometheus should bind to";
      };
    };
    user = mkOption {
      type = types.str;
      default = "funnel";
      description = "systemd user";
    };
    group = mkOption {
      type = types.str;
      default = "funnel";
      description = "group of systemd user";
    };
  };

  config = lib.mkIf cfg.enable {
    systemd = {
      services = {
        "funnel" = {
          enable = true;
          wantedBy = [ "multi-user.target" ];

          script = "exec ${pkgs.funnel}/bin/funnel &";

          environment = {
            "GRPC_PORT" = "${toString cfg.GRPC.port}";
            "WEBSOCKET_PORT" = "${toString cfg.defaultWebsocket.port}";
            "EXPORTER_HOST" = "${cfg.metrics.host}:${toString cfg.metrics.port}";
          };

          serviceConfig = {
            Type = "forking";
            User = "${cfg.user}";
            Restart = "always";
          };
        };
      };
    };

    users.users = {
      "${cfg.user}" = {
        name = "${cfg.user}";
        description = "public websocket service user";
        isSystemUser = true;
        group = "${cfg.group}";
        extraGroups = [ ];
      };
    };
  };
}
