-- NREPlatform LuCI Controller
-- Spec section 28: LuCI Integration

module("luci.controller.nreplatform", package.seeall)

function index()
    entry({"admin", "nreplatform"}, alias("admin", "nreplatform", "dashboard"), _("NREPlatform"), 60)
    entry({"admin", "nreplatform", "dashboard"}, template("nreplatform/dashboard"), _("Dashboard"), 1)
    entry({"admin", "nreplatform", "experiments"}, template("nreplatform/experiments"), _("Experiments"), 2)
    entry({"admin", "nreplatform", "profiles"}, template("nreplatform/profiles"), _("Profiles"), 3)
    entry({"admin", "nreplatform", "policies"}, template("nreplatform/policies"), _("Policies"), 4)
    entry({"admin", "nreplatform", "monitoring"}, template("nreplatform/monitoring"), _("Monitoring"), 5)
    entry({"admin", "nreplatform", "history"}, template("nreplatform/history"), _("History"), 6)
    entry({"admin", "nreplatform", "settings"}, template("nreplatform/settings"), _("Settings"), 7)

    -- API endpoints
    entry({"admin", "nreplatform", "api", "status"}, call("api_status"), nil)
    entry({"admin", "nreplatform", "api", "experiments"}, call("api_experiments"), nil)
    entry({"admin", "nreplatform", "api", "start_experiment"}, call("api_start_experiment"), nil)
    entry({"admin", "nreplatform", "api", "profiles"}, call("api_profiles"), nil)
    entry({"admin", "nreplatform", "api", "policies"}, call("api_policies"), nil)
    entry({"admin", "nreplatform", "api", "history"}, call("api_history"), nil)
end

function api_status()
    local status = {}
    local fd = io.popen("/usr/local/bin/nre status 2>/dev/null")
    if fd then
        status.raw = fd:read("*a")
        fd:close()
    end
    luci.http.prepare_content("application/json")
    luci.http.write_json(status)
end

function api_experiments()
    local profiles = {}
    local fd = io.popen("/usr/local/bin/nre profile list 2>/dev/null")
    if fd then
        for line in fd:lines() do
            local name = line:match("^%s*-%s*(.+)")
            if name then
                table.insert(profiles, name)
            end
        end
        fd:close()
    end
    luci.http.prepare_content("application/json")
    luci.http.write_json(profiles)
end

function api_start_experiment()
    local name = luci.http.formvalue("name")
    local iface = luci.http.formvalue("iface") or "eth0"
    if name then
        os.execute("/usr/local/bin/nre profile start --profile-name '" .. name .. "' --iface '" .. iface .. "' &")
        luci.http.prepare_content("application/json")
        luci.http.write_json({status = "started", profile = name})
    else
        luci.http.status(400)
        luci.http.prepare_content("application/json")
        luci.http.write_json({error = "Missing profile name"})
    end
end

function api_profiles()
    luci.http.prepare_content("application/json")
    luci.http.write_json({})
end

function api_policies()
    luci.http.prepare_content("application/json")
    luci.http.write_json({})
end

function api_history()
    luci.http.prepare_content("application/json")
    luci.http.write_json({})
end
