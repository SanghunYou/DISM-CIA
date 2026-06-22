/* =========================================================
   Dashboard ESP32 + LittleFS
   Sistema Automatizado de Cama de Inmersión
   ========================================================= */

const API = {
  status: "/api/status",
  setMode: "/api/setMode",
  manual: "/api/manual",
  setThresholds: "/api/setThresholds",
  startAuto: "/api/startAuto",
  stop: "/api/stop",
  reset: "/api/reset"
};

const STATE = {
  mode: "manual", // "manual" o "auto"
  autoStarted: false,
  data: {
    state: "Monitoreando",
    manualStep: "",
    avgHumidity: 0,
    soil: [0, 0, 0, 0],
    soilRaw: [0, 0, 0, 0],
    soilPins: [34, 35, 36, 39],
    temperature: 0,
    airHumidity: 0,
    dhtError: true,
    emergencyStop: false,
    physicalEmergency: false,
    webEmergency: false,
    errorActive: false,
    emergencySource: "none",
    alert: "",
    levelError: false,
    buttons: { emergency: false, fill: false, drain: false, fan: false },
    actuators: { valve: false, fillPump: false, drainPump: false, fan: false },
    levels: {
      bedLow: false,
      bedHigh: false,
      tankHasMinimumWater: false,
      tankHigh: false,
      bedLevelError: false,
      tankLevelError: false
    }
  },
  autoSettings: {
    fillStart: 30,
    irrigationTarget: 80,
    ventEnd: 75
  },
  webManualCommand: "none",
  dataHistory: [],
  eventLog: []
};

let lastDataSampleTime = 0;
let lastLoggedState = null;
let currentStateStartTime = Date.now();
let apiOnline = false;

const DATA_SAMPLE_INTERVAL_MS = 5000;
const STATUS_REFRESH_INTERVAL_MS = 1000;
const MAX_DATA_POINTS = 360; // 30 min con muestra cada 5 s
const MAX_EVENT_LOG = 40;

const cycleDurations = {
  fill: 0,
  capillary: 0,
  drain: 0,
  vent: 0
};

/* ===================== API ===================== */

async function apiGet(path) {
  const response = await fetch(path, { cache: "no-store" });
  if (!response.ok) {
    throw new Error(`HTTP ${response.status} en ${path}`);
  }
  return response;
}

async function fetchStatus() {
  let status = null;

  try {
    const response = await apiGet(API.status);
    status = await response.json();
    apiOnline = true;
  } catch (error) {
    apiOnline = false;
    showConnectionWarning(error);
    console.error("Error leyendo /api/status:", error);
    return;
  }

  try {
    applyStatusFromESP(status);
    updateDashboard();
  } catch (error) {
    console.error("Error renderizando dashboard:", error);
    console.log("Status recibido correctamente:", status);
  }
}

function showConnectionWarning(error) {
  const alertBox = document.getElementById("systemAlert");
  const alertTitle = document.getElementById("alertTitle");
  const alertMessage = document.getElementById("alertMessage");
  const alertActionBtn = document.getElementById("alertActionBtn");

  if (!alertBox || STATE.data.emergencyStop || STATE.data.levelError) return;

  alertBox.classList.remove("hidden");
  alertBox.classList.add("warning");
  alertTitle.textContent = "Sin comunicación con ESP32";
  alertMessage.textContent = "No se pudo leer /api/status. Revisa que estés conectado a la red del ESP32.";
  alertActionBtn.textContent = "Reintentar";
  alertActionBtn.onclick = () => fetchStatus();
  console.warn(error);
}

function applyStatusFromESP(status) {
  STATE.mode = normalizeMode(status.mode);

  const levels = status.levels || {};
  const buttons = status.buttons || {};
  const actuators = status.actuators || {};
  const dht = status.dht || {};
  const soilArray = status.soil || [];
  const thresholds = status.thresholds || {};

  STATE.data.state = normalizeCycleState(status.state || "Monitoreando");
  STATE.data.manualStep = status.manualStep || "";
  STATE.data.avgHumidity = Number(status.soilAverage ?? 0);
  
  STATE.data.soil = soilArray.map(item => Number(item.percent ?? 0));
  STATE.data.soilRaw = soilArray.map(item => Number(item.raw ?? 0));
  STATE.data.soilPins = soilArray.map(item => Number(item.pin ?? 0));

  while (STATE.data.soil.length < 4) STATE.data.soil.push(0);
  while (STATE.data.soilRaw.length < 4) STATE.data.soilRaw.push(0);
  while (STATE.data.soilPins.length < 4) {
    STATE.data.soilPins.push([34, 35, 36, 39][STATE.data.soilPins.length]);
  }

  STATE.data.temperature = Number(dht.temperature ?? 0);
  STATE.data.airHumidity = Number(dht.humidity ?? 0);
  STATE.data.dhtError = !!dht.error;

  STATE.autoStarted = !!status.autoStarted;
  STATE.data.alert = status.alert || status.message || "";
  STATE.data.physicalEmergency = !!status.isPhysicalEmergency || !!buttons.emergency;
  STATE.data.webEmergency = !!status.isWebStopRequested;
  STATE.data.errorActive = !!status.isErrorActive;
  STATE.data.emergencySource = status.emergencySource || (STATE.data.physicalEmergency ? "physical" : (STATE.data.webEmergency ? "web" : "none"));

  STATE.data.levels = {
    bedLow: !!levels.bedLow,
    bedHigh: !!levels.bedHigh,
    tankHasMinimumWater: !!levels.tankHasMinimumWater,
    tankHigh: !!levels.tankHigh,
    bedLevelError: !!levels.bedLevelError,
    tankLevelError: !!levels.tankLevelError
  };

  STATE.data.buttons = {
    emergency: !!buttons.emergency,
    fill: !!buttons.fill,
    drain: !!buttons.drain,
    fan: !!buttons.fan
  };

  STATE.data.actuators = {
    valve: !!actuators.valve,
    fillPump: !!actuators.fillPump,
    drainPump: !!actuators.drainPump,
    fan: !!actuators.fan
  };

  STATE.data.emergencyStop = STATE.data.physicalEmergency || STATE.data.webEmergency || !!status.isEmergencyActive;
  STATE.data.levelError = STATE.data.errorActive || STATE.data.levels.bedLevelError || STATE.data.levels.tankLevelError;

  if (typeof thresholds.fillStart !== "undefined") STATE.autoSettings.fillStart = Number(thresholds.fillStart);
  if (typeof thresholds.irrigationTarget !== "undefined") STATE.autoSettings.irrigationTarget = Number(thresholds.irrigationTarget);
  if (typeof thresholds.ventEnd !== "undefined") STATE.autoSettings.ventEnd = Number(thresholds.ventEnd);
}

function normalizeMode(mode) {
  if (!mode) return STATE.mode || "manual";
  const m = String(mode).toLowerCase();
  if (m.includes("auto")) return "auto";
  return "manual";
}

/* ===================== HELPERS DE ESTADO/NIVEL ===================== */

function normalizeCycleState(stateName) {
  if (!stateName) return "Monitoreando";

  const s = String(stateName).trim();
  if (s === "Espera") return "Monitoreando";
  if (s === "Monitoreando humedad") return "Monitoreando";
  if (s === "Regando") return "Regando por capilaridad";
  if (s === "Paro/Emergencia") return "Paro";

  return s;
}

function getOrderedStates() {
  if (STATE.mode === "auto") {
    return ["Monitoreando", "Llenando", "Regando por capilaridad", "Drenando", "Ventilando"];
  }
  return ["Monitoreando", "Llenando", "Drenando", "Ventilando"];
}

function getNextState(currentState) {
  const states = getOrderedStates();
  const index = states.indexOf(currentState);
  if (index === -1) return states[0];
  return states[(index + 1) % states.length];
}

function getReadableStateName(stateName) {
  if (stateName === "Monitoreando") return "Monitoreo";
  if (stateName === "Llenando") return "Llenado";
  if (stateName === "Regando por capilaridad") return "Capilaridad";
  if (stateName === "Drenando") return "Drenado";
  if (stateName === "Ventilando") return "Ventilación";
  if (stateName === "Paro") return "Paro";
  if (stateName === "Error") return "Error";
  return stateName;
}

function getCurrentStateSubtitle(stateName) {
  if (stateName === "Monitoreando") return "Analizando humedad y niveles";
  if (stateName === "Llenando") return "Bomba de llenado activa";
  if (stateName === "Regando por capilaridad") return "Absorción del sustrato";
  if (stateName === "Drenando") return "Vaciando cama hacia depósito";
  if (stateName === "Ventilando") return "Reduciendo humedad residual";
  if (stateName === "Paro") return "Sistema detenido";
  if (stateName === "Error") return "Revisión requerida";
  return "Estado operativo";
}

function getCurrentVisualClass(stateName) {
  if (stateName === "Monitoreando") return "monitoring";
  if (stateName === "Llenando") return "filling";
  if (stateName === "Regando por capilaridad") return "capillary";
  if (stateName === "Drenando") return "draining";
  if (stateName === "Ventilando") return "venting";
  return "";
}

function getBedLevelFromStatus() {
  const l = STATE.data.levels;
  if (l.bedLevelError) return "Error";
  if (l.bedHigh) return "Lleno";
  if (l.bedLow) return "Vacío";
  return "Subiendo";
}

function getTankLevelFromStatus() {
  const l = STATE.data.levels;
  if (l.tankLevelError) return "Error";
  if (l.tankHigh) return "Alto";
  if (l.tankHasMinimumWater) return "Medio";
  return "Bajo";
}

function formatDuration(ms) {
  const totalSeconds = Math.max(0, Math.floor(ms / 1000));
  const minutes = Math.floor(totalSeconds / 60);
  const seconds = totalSeconds % 60;
  return `${minutes}m ${seconds}s`;
}

function getCurrentTimeLabel() {
  return new Date().toLocaleTimeString("es-MX", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit"
  });
}

function setElementActive(id, isActive, className = "is-on") {
  const el = document.getElementById(id);
  if (!el) return;
  el.classList.toggle(className, !!isActive);
}

/* ===================== DASHBOARD PRINCIPAL ===================== */

function updateDashboard() {
  const bedLevel = getBedLevelFromStatus();
  const tankLevel = getTankLevelFromStatus();

  renderCycleState();
  renderAlert();
  updateAverageHumidity(STATE.data.avgHumidity);
  updateLevelList("bedLevelList", bedLevel);
  updateLevelList("tankLevelList", tankLevel);
  updateMiniTank("bedFill", bedLevel, "bed");
  updateMiniTank("tankFill", tankLevel, "tank");
  updateAmbient(STATE.data.temperature, STATE.data.airHumidity);
  renderSupervisionViews();
  renderControlPanel();
  renderDataTab();
}

function renderAlert() {
  const alertBox = document.getElementById("systemAlert");
  const alertTitle = document.getElementById("alertTitle");
  const alertMessage = document.getElementById("alertMessage");
  const alertActionBtn = document.getElementById("alertActionBtn");
  if (!alertBox || !alertTitle || !alertMessage || !alertActionBtn) return;

  alertActionBtn.disabled = false;
  alertActionBtn.style.display = "inline-block";

  const alertText = STATE.data.alert || "";

  if (STATE.data.physicalEmergency) {
    alertBox.classList.remove("hidden");
    alertBox.classList.remove("warning");
    alertTitle.textContent = "Paro físico activo";
    alertMessage.textContent = alertText || "El paro físico está activo. El sistema conserva el modo y el estado actual.";
    alertActionBtn.textContent = "Esperando liberación física";
    alertActionBtn.disabled = true;
    alertActionBtn.onclick = null;
    return;
  }

  if (STATE.data.webEmergency) {
    alertBox.classList.remove("hidden");
    alertBox.classList.remove("warning");
    alertTitle.textContent = "Paro web activo";
    alertMessage.textContent = alertText || "El paro fue solicitado desde la página. El sistema conserva el modo y el estado actual.";
    alertActionBtn.textContent = "Liberar paro web";
    alertActionBtn.onclick = () => releaseEmergencyStopToManual();
    return;
  }

  if (STATE.data.errorActive || STATE.data.levelError) {
    alertBox.classList.remove("hidden");
    alertBox.classList.add("warning");
    alertTitle.textContent = "Error del sistema";
    alertMessage.textContent = alertText || "Se detectó una condición de error. El sistema conserva el modo y el estado actual.";
    alertActionBtn.textContent = "Revisado / Reset";
    alertActionBtn.onclick = () => resetSystem();
    return;
  }

  if (alertText.toLowerCase().includes("fuga")) {
    alertBox.classList.remove("hidden");
    alertBox.classList.add("warning");
    alertTitle.textContent = "Posible fuga detectada";
    alertMessage.textContent = alertText;
    alertActionBtn.textContent = "Entendido";
    alertActionBtn.onclick = () => alertBox.classList.add("hidden");
    return;
  }
  if (alertText && alertText.trim() !== "") {
    alertBox.classList.remove("hidden");
    alertBox.classList.remove("warning");
    alertTitle.textContent = "Aviso del sistema";
    alertMessage.textContent = alertText;
    alertActionBtn.textContent = "Entendido";
    alertActionBtn.onclick = () => alertBox.classList.add("hidden");
    return;
  }
  if (apiOnline) {
    alertBox.classList.add("hidden");
  }
}

function renderCycleState() {
  const currentState = normalizeCycleState(STATE.data.state);
  const nextState = getNextState(currentState);

  const items = document.querySelectorAll(".cycle-item");
  const currentBox = document.getElementById("currentStateBox");
  const currentTitle = document.getElementById("currentStateTitle");
  const currentSubtitle = document.getElementById("currentStateSubtitle");

  if (!currentBox || !currentTitle || !currentSubtitle) return;

  currentBox.classList.remove("monitoring", "filling", "capillary", "draining", "venting");
  const visualClass = getCurrentVisualClass(currentState);
  if (visualClass) currentBox.classList.add(visualClass);

  currentTitle.textContent = getReadableStateName(currentState);
  currentSubtitle.textContent = getCurrentStateSubtitle(currentState);

  items.forEach(item => {
    const itemState = item.dataset.state;
    item.classList.remove("active", "next", "disabled");

    if (STATE.mode === "manual" && itemState === "Regando por capilaridad") {
      item.classList.add("disabled");
    }

    if (itemState === currentState) item.classList.add("active");
    if (itemState === nextState && currentState !== "Paro" && currentState !== "Error") item.classList.add("next");
  });
}

function updateAverageHumidity(value) {
  const pct = Math.max(0, Math.min(100, Number(value || 0)));
  const orb = document.getElementById("humidityOrb");
  const text = document.getElementById("avgHumidity");
  if (!orb || !text) return;

  text.textContent = Math.round(pct);
  orb.style.setProperty("--water-level", `${pct}%`);
  orb.style.setProperty("--orb-rotation", `${pct * 3.6}deg`);
}

function updateLevelList(listId, level) {
  const items = document.querySelectorAll(`#${listId} li`);
  items.forEach(item => {
    item.classList.remove("active", "warning", "good");
    if (item.dataset.level === level) {
      item.classList.add("active");
      if (level === "Vacío" || level === "Bajo" || level === "Error") item.classList.add("warning");
      if (level === "Lleno" || level === "Alto") item.classList.add("good");
    }
  });
}

function updateMiniTank(fillId, level, type) {
  const fill = document.getElementById(fillId);
  if (!fill || !fill.parentElement) return;

  const tank = fill.parentElement;
  let height = "10%";
  let color = "linear-gradient(180deg, rgba(66,165,245,0.7), rgba(21,101,192,0.92))";

  if (type === "bed") {
    if (level === "Vacío") height = "8%";
    if (level === "Subiendo") height = "52%";
    if (level === "Lleno") height = "88%";
    if (level === "Error") {
      height = "50%";
      color = "linear-gradient(180deg, rgba(239,83,80,0.65), rgba(183,28,28,0.9))";
    }
  }

  if (type === "tank") {
    if (level === "Bajo") {
      height = "22%";
      color = "linear-gradient(180deg, rgba(239,83,80,0.65), rgba(183,28,28,0.9))";
    }
    if (level === "Medio") {
      height = "55%";
      color = "linear-gradient(180deg, rgba(249,168,37,0.65), rgba(245,127,23,0.9))";
    }
    if (level === "Alto") height = "88%";
    if (level === "Error") {
      height = "50%";
      color = "linear-gradient(180deg, rgba(239,83,80,0.65), rgba(183,28,28,0.9))";
    }
  }

  fill.style.height = height;
  fill.style.background = color;
  tank.style.setProperty("--level-height", height);
}

function updateAmbient(temp, humidity) {
  const t = document.getElementById("temperature");
  const h = document.getElementById("airHumidity");
  if (t) t.textContent = STATE.data.dhtError ? "--" : Number(temp || 0).toFixed(1);
  if (h) h.textContent = STATE.data.dhtError ? "--" : Number(humidity || 0).toFixed(1);
}

/* ===================== SUPERVISIÓN ===================== */

function getActuatorStatus() {
  return STATE.data.actuators || { valve: false, fillPump: false, drainPump: false, fan: false };
}

function renderSupervisionViews() {
  const soil = STATE.data.soil || [0, 0, 0];
  const bedLevel = getBedLevelFromStatus();
  const tankLevel = getTankLevelFromStatus();
  const actuators = getActuatorStatus();
  const l = STATE.data.levels;

  ["valSH1", "valSH2", "valSH3", "valSH4"].forEach((id, index) => {
  const el = document.getElementById(id);
  if (el) el.textContent = `${Math.round(soil[index] || 0)}%`;
});

  setElementActive("actValveTop", actuators.valve);
  setElementActive("actFillPumpTop", actuators.fillPump);
  setElementActive("actDrainPumpTop", actuators.drainPump);
  setElementActive("fan1Top", actuators.fan);
  setElementActive("fan2Top", actuators.fan);
  setElementActive("fan3Top", actuators.fan);

  setElementActive("lvlBedLow", l.bedLow, "is-active");
  setElementActive("lvlBedHigh", l.bedHigh, "is-active");
  setElementActive("lvlTankLow", l.tankHasMinimumWater, "is-active");
  setElementActive("lvlTankHigh", l.tankHigh, "is-active");

  const bedSideWater = document.getElementById("bedSideWater");
  if (bedSideWater) {
    let bedHeight = "8%";
    if (bedLevel === "Subiendo") bedHeight = "46%";
    if (bedLevel === "Lleno") bedHeight = "88%";
    if (bedLevel === "Error") bedHeight = "50%";
    bedSideWater.style.height = bedHeight;
  }

  const tankSideWater = document.getElementById("tankSideWater");
  if (tankSideWater) {
    let tankHeight = "18%";
    if (tankLevel === "Medio") tankHeight = "50%";
    if (tankLevel === "Alto") tankHeight = "86%";
    if (tankLevel === "Error") tankHeight = "50%";
    tankSideWater.style.height = tankHeight;
  }
}

/* ===================== TABS ===================== */

document.querySelectorAll(".tab").forEach(button => {
  button.addEventListener("click", () => {
    document.querySelectorAll(".tab").forEach(b => b.classList.remove("active"));
    document.querySelectorAll(".tab-content").forEach(p => p.classList.remove("active"));

    button.classList.add("active");
    document.getElementById(`tab-${button.dataset.tab}`).classList.add("active");
  });
});

/* ===================== CONTROL ===================== */

async function toggleControlMode() {
  const isAuto = document.getElementById("controlModeToggle").checked;
  const mode = isAuto ? "auto" : "manual";

  try {
    await apiGet(`${API.setMode}?mode=${mode}`);
    await fetchStatus();
  } catch (error) {
    console.error(error);
    alert("No se pudo cambiar el modo de operación.");
    await fetchStatus();
  }
}

async function manualControlAction(action) {
  if (STATE.mode !== "manual") return;

  try {
    await apiGet(`${API.manual}?action=${action}`);
    await fetchStatus();
  } catch (error) {
    console.error(error);
    alert("No se pudo enviar el comando manual.");
  }
}

async function clearManualControl() {
  try {
    await apiGet(`${API.manual}?action=clear`);
    await fetchStatus();
  } catch (error) {
    console.error(error);
    alert("No se pudo desenclavar la acción web.");
  }
}

async function saveAutoSettings() {
  const fillStart = parseInt(document.getElementById("fillStartInput").value);
  const irrigationTarget = parseInt(document.getElementById("irrigationTargetInput").value);
  const ventEnd = parseInt(document.getElementById("ventEndInput").value);

  if (isNaN(fillStart) || isNaN(irrigationTarget) || isNaN(ventEnd)) {
    alert("Todos los valores deben ser números.");
    return;
  }

  if (
    fillStart < 0 || fillStart > 100 ||
    irrigationTarget < 0 || irrigationTarget > 100 ||
    ventEnd < 0 || ventEnd > 100
  ) {
    alert("Todos los valores deben estar entre 0 y 100%.");
    return;
  }

  if (fillStart >= irrigationTarget) {
    alert("Inicio de llenado debe ser menor que objetivo de riego.");
    return;
  }

  if (ventEnd >= irrigationTarget) {
    alert("Fin de ventilación debe ser menor que objetivo de riego.");
    return;
  }

  try {
    await apiGet(`${API.setThresholds}?fillStart=${fillStart}&irrigationTarget=${irrigationTarget}&ventEnd=${ventEnd}`);
    STATE.autoSettings.fillStart = fillStart;
    STATE.autoSettings.irrigationTarget = irrigationTarget;
    STATE.autoSettings.ventEnd = ventEnd;
    updateSavedThresholdLabels();
    await fetchStatus();
    alert("Configuración automática guardada.");
  } catch (error) {
    console.error(error);
    alert("No se pudo guardar la configuración automática.");
  }
}

function renderControlPanel() {
  const isAuto = STATE.mode === "auto";

  const toggle = document.getElementById("controlModeToggle");
  const manualLabel = document.getElementById("controlManualLabel");
  const autoLabel = document.getElementById("controlAutoLabel");
  const modeText = document.getElementById("controlModeText");
  const manualPanel = document.getElementById("manualControlPanel");
  const autoPanel = document.getElementById("autoControlPanel");
  const startAutoBtn = document.getElementById("startAutoBtn");
  const autoStartStatus = document.getElementById("autoStartStatus");

  if (!toggle || !manualLabel || !autoLabel || !modeText || !manualPanel || !autoPanel) return;

  toggle.checked = isAuto;
  manualLabel.classList.toggle("active", !isAuto);
  autoLabel.classList.toggle("active", isAuto);

  if (isAuto && STATE.autoStarted) {
    modeText.textContent = "Modo automático en ejecución. Ante paro o error se conserva el modo y estado actual.";
  } else if (isAuto) {
    modeText.textContent = "Modo automático seleccionado. Presiona EMPEZAR para iniciar el ciclo.";
  } else {
    modeText.textContent = "Modo manual habilitado. Usa los botones respetando la secuencia de operación.";
  }

  manualPanel.classList.toggle("panel-disabled", isAuto);
  autoPanel.classList.toggle("panel-disabled", !isAuto);

  if (startAutoBtn) {
    const disabled = !isAuto || STATE.autoStarted || STATE.data.emergencyStop || STATE.data.levelError;
    startAutoBtn.disabled = disabled;
    startAutoBtn.classList.toggle("disabled", disabled);
  }

  if (autoStartStatus) {
    if (!isAuto) {
      autoStartStatus.textContent = "Selecciona modo AUTO para habilitar el arranque automático.";
    } else if (STATE.autoStarted) {
      autoStartStatus.textContent = "Modo automático iniciado.";
    } else {
      autoStartStatus.textContent = "Modo automático seleccionado, pendiente de iniciar.";
    }
  }

  renderManualControlButtons();
  renderAutoConfigInputs();
}

function renderManualControlButtons() {
  const fillBtn = document.getElementById("manualFillBtn");
  const drainBtn = document.getElementById("manualDrainBtn");
  const fanBtn = document.getElementById("manualFanBtn");

  if (!fillBtn || !drainBtn || !fanBtn) return;

  fillBtn.classList.remove("active", "disabled");
  drainBtn.classList.remove("active", "disabled");
  fanBtn.classList.remove("active", "disabled");

  fillBtn.disabled = false;
  drainBtn.disabled = false;
  fanBtn.disabled = false;

  if (STATE.mode !== "manual") {
    fillBtn.classList.add("disabled");
    drainBtn.classList.add("disabled");
    fanBtn.classList.add("disabled");

    fillBtn.disabled = true;
    drainBtn.disabled = true;
    fanBtn.disabled = true;
    return;
  }

  const currentState = normalizeCycleState(STATE.data.state);
  const manualStep = STATE.data.manualStep || "";

  if (currentState === "Llenando") fillBtn.classList.add("active");
  if (currentState === "Drenando") drainBtn.classList.add("active");
  if (currentState === "Ventilando") fanBtn.classList.add("active");

  const canFill = manualStep === "Llenar cama";
  const canDrain = true;
  const canFan = manualStep === "Ventilar" || manualStep === "Llenar cama";

  if (!canFill) {
    fillBtn.classList.add("disabled");
    fillBtn.disabled = true;
  }

  if (!canDrain) {
    drainBtn.classList.add("disabled");
    drainBtn.disabled = true;
  }

  if (!canFan) {
    fanBtn.classList.add("disabled");
    fanBtn.disabled = true;
  }
}

function renderAutoConfigInputs() {
  const fillStartInput = document.getElementById("fillStartInput");
  const irrigationTargetInput = document.getElementById("irrigationTargetInput");
  const ventEndInput = document.getElementById("ventEndInput");

  if (!fillStartInput || !irrigationTargetInput || !ventEndInput) return;

  if (document.activeElement !== fillStartInput) {
    fillStartInput.value = STATE.autoSettings.fillStart;
  }

  if (document.activeElement !== irrigationTargetInput) {
    irrigationTargetInput.value = STATE.autoSettings.irrigationTarget;
  }

  if (document.activeElement !== ventEndInput) {
    ventEndInput.value = STATE.autoSettings.ventEnd;
  }

  updateSavedThresholdLabels();
}

function updateSavedThresholdLabels() {
  const savedFillStart = document.getElementById("savedFillStart");
  const savedIrrigationTarget = document.getElementById("savedIrrigationTarget");
  const savedVentEnd = document.getElementById("savedVentEnd");

  if (savedFillStart) savedFillStart.textContent = `${STATE.autoSettings.fillStart}%`;
  if (savedIrrigationTarget) savedIrrigationTarget.textContent = `${STATE.autoSettings.irrigationTarget}%`;
  if (savedVentEnd) savedVentEnd.textContent = `${STATE.autoSettings.ventEnd}%`;
}

async function startAutoMode() {
  try {
    await apiGet(API.startAuto);
    await fetchStatus();
  } catch (error) {
    console.error(error);
    alert("No se pudo iniciar el modo automático.");
  }
}

async function requestStop() {
  try {
    await apiGet(API.stop);
    await fetchStatus();
  } catch (error) {
    console.error(error);
    alert("No se pudo activar el paro web.");
  }
}

async function resetSystem() {
  try {
    await apiGet(API.reset);
    resetCycleTimers();
    await fetchStatus();
  } catch (error) {
    console.error(error);
    alert("No se pudo reiniciar el sistema.");
  }
}

async function releaseEmergencyStopToManual() {
  await resetSystem();
}

document.getElementById("btnStop")?.addEventListener("click", requestStop);
document.getElementById("btnReset")?.addEventListener("click", resetSystem);

/* ===================== DATOS ===================== */

function createDataSnapshot() {
  return {
    time: Date.now(),
    label: getCurrentTimeLabel(),
    state: normalizeCycleState(STATE.data.state),
    mode: STATE.mode,
    avgHumidity: Math.round(STATE.data.avgHumidity || 0),
    temperature: Number(STATE.data.temperature || 0),
    airHumidity: Number(STATE.data.airHumidity || 0),
    soil: STATE.data.soil || [0, 0, 0],
    bedLevel: getBedLevelFromStatus(),
    tankLevel: getTankLevelFromStatus()
  };
}

function collectDataPointIfNeeded() {
  const now = Date.now();
  if (now - lastDataSampleTime < DATA_SAMPLE_INTERVAL_MS) return;
  lastDataSampleTime = now;

  STATE.dataHistory.push(createDataSnapshot());
  if (STATE.dataHistory.length > MAX_DATA_POINTS) STATE.dataHistory.shift();
}

function logStateEventIfNeeded() {
  const currentState = normalizeCycleState(STATE.data.state);
  if (currentState === lastLoggedState) return;

  const now = Date.now();

  if (lastLoggedState !== null) {
    const elapsed = now - currentStateStartTime;
    if (lastLoggedState === "Llenando") cycleDurations.fill += elapsed;
    if (lastLoggedState === "Regando por capilaridad") cycleDurations.capillary += elapsed;
    if (lastLoggedState === "Drenando") cycleDurations.drain += elapsed;
    if (lastLoggedState === "Ventilando") cycleDurations.vent += elapsed;
  }

  currentStateStartTime = now;
  lastLoggedState = currentState;

  STATE.eventLog.unshift(createDataSnapshot());
  if (STATE.eventLog.length > MAX_EVENT_LOG) STATE.eventLog.pop();
}

function resetCycleTimers() {
  cycleDurations.fill = 0;
  cycleDurations.capillary = 0;
  cycleDurations.drain = 0;
  cycleDurations.vent = 0;
  currentStateStartTime = Date.now();
  lastLoggedState = null;
}

function valueToChartY(value, min, max) {
  const safeValue = Math.max(min, Math.min(max, Number(value || 0)));
  return 38 - ((safeValue - min) / (max - min)) * 34;
}

function buildPolylinePoints(values, min, max) {
  if (!values || values.length === 0) return "";
  if (values.length === 1) {
    const y = valueToChartY(values[0], min, max);
    return `0,${y} 100,${y}`;
  }

  return values.map((value, index) => {
    const x = (index / (values.length - 1)) * 100;
    const y = valueToChartY(value, min, max);
    return `${x.toFixed(2)},${y.toFixed(2)}`;
  }).join(" ");
}

function renderSingleLineChart(svgId, values, min, max, className) {
  const svg = document.getElementById(svgId);
  if (!svg) return;
  const points = buildPolylinePoints(values, min, max);
  svg.innerHTML = `<polyline class="chart-line ${className}" points="${points}"></polyline>`;
}

function renderDoubleLineChart(svgId, valuesA, valuesB, min, max) {
  const svg = document.getElementById(svgId);
  if (!svg) return;
  const pointsA = buildPolylinePoints(valuesA, min, max);
  const pointsB = buildPolylinePoints(valuesB, min, max);
  svg.innerHTML = `
    <polyline class="chart-line temp" points="${pointsA}"></polyline>
    <polyline class="chart-line air" points="${pointsB}"></polyline>
  `;
}

function renderDataCharts() {
  const history = STATE.dataHistory;

  renderSingleLineChart(
    "avgHumidityChart",
    history.map(item => item.avgHumidity),
    0,
    100,
    "avg"
  );

  const avgNow = document.getElementById("dataAvgNow");

  if (avgNow) {
    avgNow.textContent = `${Math.round(STATE.data.avgHumidity || 0)}%`;
  }
}

function renderCycleSummary() {
  const now = Date.now();
  const currentState = normalizeCycleState(STATE.data.state);
  const currentElapsed = now - currentStateStartTime;

  let fillTime = cycleDurations.fill;
  let capillaryTime = cycleDurations.capillary;
  let drainTime = cycleDurations.drain;
  let ventTime = cycleDurations.vent;

  if (currentState === "Llenando") fillTime += currentElapsed;
  if (currentState === "Regando por capilaridad") capillaryTime += currentElapsed;
  if (currentState === "Drenando") drainTime += currentElapsed;
  if (currentState === "Ventilando") ventTime += currentElapsed;

  const setText = (id, value) => {
    const el = document.getElementById(id);
    if (el) el.textContent = value;
  };

  setText("summaryCurrentDuration", formatDuration(currentElapsed));
  setText("summaryFillTime", formatDuration(fillTime));
  setText("summaryCapillaryTime", formatDuration(capillaryTime));
  setText("summaryDrainTime", formatDuration(drainTime));
  setText("summaryVentTime", formatDuration(ventTime));
  setText("summarySamples", `${STATE.dataHistory.length} / ${MAX_DATA_POINTS}`);
}

function renderEventLog() {
  const tbody = document.getElementById("eventLogTable");
  if (!tbody) return;

  if (STATE.eventLog.length === 0) {
    tbody.innerHTML = `<tr><td colspan="5">Sin eventos registrados todavía.</td></tr>`;
    return;
  }

  tbody.innerHTML = STATE.eventLog.map(event => `
    <tr>
      <td>${event.label}</td>
      <td><span class="event-state-pill">${getReadableStateName(event.state)}</span></td>
      <td>${event.avgHumidity}%</td>
      <td>${event.bedLevel}</td>
      <td>${event.tankLevel}</td>
    </tr>
  `).join("");
}

function createDiagRow(label, value, trueText = "ACTIVO", falseText = "INACTIVO") {
  const activeClass = value ? "is-on" : "";
  const text = value ? trueText : falseText;
  return `
    <div class="diag-row ${activeClass}">
      <div>
        <strong>${label}</strong><br>
        <span>${text}</span>
      </div>
      <span class="diag-dot"></span>
    </div>
  `;
}

function renderDiagnosticPanel() {
  const soil = STATE.data.soil || [0, 0, 0];
  const soilRaw = STATE.data.soilRaw || [0, 0, 0];
  const soilPins = STATE.data.soilPins || [34, 36, 35];
  const soilTable = document.getElementById("diagSoilTable");

  if (soilTable) {
    soilTable.innerHTML = soil.map((value, index) => `
      <tr>
        <td>D${soilPins[index] ?? "--"}</td>
        <td>${soilRaw[index] ?? "--"}</td>
        <td>${Math.round(value || 0)}%</td>
      </tr>
    `).join("");
  }

  const buttons = document.getElementById("diagButtons");
  if (buttons) {
    const b = STATE.data.buttons || {};
    buttons.innerHTML = `
      ${createDiagRow("D25 Paro físico", b.emergency, "PARO ACTIVO", "LIBRE")}
      ${createDiagRow("D26 Botón llenado", b.fill, "ENCLAVADO", "LIBRE")}
      ${createDiagRow("D27 Botón drenado", b.drain, "ENCLAVADO", "LIBRE")}
      ${createDiagRow("D14 Botón ventilación", b.fan, "ENCLAVADO", "LIBRE")}
    `;
  }
}

function renderDataTab() {
  collectDataPointIfNeeded();
  logStateEventIfNeeded();
  renderDataCharts();
  renderCycleSummary();
  renderEventLog();
  renderDiagnosticPanel();
}

/* ===================== INICIO ===================== */

window.addEventListener("load", () => {
  updateSavedThresholdLabels();
  fetchStatus();
  setInterval(fetchStatus, STATUS_REFRESH_INTERVAL_MS);
});
