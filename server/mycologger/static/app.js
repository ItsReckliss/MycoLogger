const ui = {
  navItems: [...document.querySelectorAll(".nav-item")],
  views: [...document.querySelectorAll(".view")],
  openViewButtons: [...document.querySelectorAll("[data-open-view]")],
  viewTitle: document.querySelector("#view-title"),
  lastRefresh: document.querySelector("#last-refresh"),
  refreshButton: document.querySelector("#refresh-button"),
  sidebar: document.querySelector("#sidebar"),
  menuButton: document.querySelector("#menu-button"),
  sidebarStatusLight: document.querySelector("#sidebar-status-light"),
  sidebarStatus: document.querySelector("#sidebar-status"),
  sidebarDetail: document.querySelector("#sidebar-detail"),
  receiverAlert: document.querySelector("#receiver-alert"),
  receiverAlertTitle: document.querySelector("#receiver-alert-title"),
  receiverAlertDetail: document.querySelector("#receiver-alert-detail"),
  receiverState: document.querySelector("#receiver-state"),
  receiverPort: document.querySelector("#receiver-port"),
  receiverFormState: document.querySelector("#receiver-form-state"),
  receiverFormPort: document.querySelector("#receiver-form-port"),
  receiverFirmware: document.querySelector("#receiver-firmware"),
  receiverResetFlags: document.querySelector("#receiver-reset-flags"),
  receiverRecordCount: document.querySelector("#receiver-record-count"),
  receiverPacketCount: document.querySelector("#receiver-packet-count"),
  receiverStoredCount: document.querySelector("#receiver-stored-count"),
  receiverDuplicateCount: document.querySelector("#receiver-duplicate-count"),
  receiverFrequency: document.querySelector("#receiver-frequency"),
  receiverModel: document.querySelector("#receiver-model"),
  receiverRadioState: document.querySelector("#receiver-radio-state"),
  nodeCount: document.querySelector("#node-count"),
  measurementCount: document.querySelector("#measurement-count"),
  growCount: document.querySelector("#grow-count"),
  growGrid: document.querySelector("#grow-grid"),
  growEmpty: document.querySelector("#grow-empty"),
  jarCount: document.querySelector("#jar-count"),
  jarGrid: document.querySelector("#jar-grid"),
  jarEmpty: document.querySelector("#jar-empty"),
  archiveTabs: [...document.querySelectorAll("[data-archive-tab]")],
  archivePanels: [...document.querySelectorAll("[data-archive-panel]")],
  pastGrowGrid: document.querySelector("#past-grow-grid"),
  pastGrowEmpty: document.querySelector("#past-grow-empty"),
  pastGrowCount: document.querySelector("#past-grow-count"),
  failedGrowGrid: document.querySelector("#failed-grow-grid"),
  failedGrowEmpty: document.querySelector("#failed-grow-empty"),
  failedGrowCount: document.querySelector("#failed-grow-count"),
  archivedJarGrid: document.querySelector("#archived-jar-grid"),
  archivedJarEmpty: document.querySelector("#archived-jar-empty"),
  archivedJarCount: document.querySelector("#archived-jar-count"),
  failedJarGrid: document.querySelector("#failed-jar-grid"),
  failedJarEmpty: document.querySelector("#failed-jar-empty"),
  failedJarCount: document.querySelector("#failed-jar-count"),
  addJar: document.querySelector("#add-jar"),
  spawnSelectedJars: document.querySelector("#spawn-selected-jars"),
  pendingCommandCount: document.querySelector("#pending-command-count"),
  commandSentCount: document.querySelector("#command-sent-count"),
  commandAckCount: document.querySelector("#command-ack-count"),
  updatedTime: document.querySelector("#updated-time"),
  overviewNodeList: document.querySelector("#overview-node-list"),
  overviewEmpty: document.querySelector("#overview-empty"),
  nodesNodeList: document.querySelector("#nodes-node-list"),
  nodesEmpty: document.querySelector("#nodes-empty"),
  nodeFilter: document.querySelector("#node-filter"),
  apiStatus: document.querySelector("#api-status"),
  browserHost: document.querySelector("#browser-host"),
  diagnosticsRefresh: document.querySelector("#diagnostics-refresh"),
  diagnosticsCount: document.querySelector("#diagnostics-count"),
  diagnosticsSummary: document.querySelector("#diagnostics-summary"),
  diagnosticsList: document.querySelector("#diagnostics-list"),
  diagnosticsEmpty: document.querySelector("#diagnostics-empty"),
  settingsDialog: document.querySelector("#node-settings-dialog"),
  settingsForm: document.querySelector("#node-settings-form"),
  settingsNodeId: document.querySelector("#settings-node-id"),
  settingsName: document.querySelector("#settings-name"),
  settingsTub: document.querySelector("#settings-tub"),
  settingsLocation: document.querySelector("#settings-location"),
  settingsNotes: document.querySelector("#settings-notes"),
  settingsActive: document.querySelector("#settings-active"),
  settingsInterval: document.querySelector("#settings-interval"),
  settingsDownlinkWindow: document.querySelector("#settings-downlink-window"),
  settingsConfigStatus: document.querySelector("#settings-config-status"),
  settingsError: document.querySelector("#settings-error"),
  settingsSave: document.querySelector("#settings-save"),
  accountButton: document.querySelector("#account-button"),
  accountDropdown: document.querySelector("#account-dropdown"),
  shutdownServer: document.querySelector("#shutdown-server"),
  loginButton: document.querySelector("#login-button"),
  tubOptions: document.querySelector("#tub-options"),
  growDialog: document.querySelector("#grow-dialog"),
  growForm: document.querySelector("#grow-form"),
  growDialogTitle: document.querySelector("#grow-dialog-title"),
  growDialogNode: document.querySelector("#grow-dialog-node"),
  growLifecycleBanner: document.querySelector("#grow-lifecycle-banner"),
  growLifecycleTitle: document.querySelector("#grow-lifecycle-title"),
  growLifecycleDetail: document.querySelector("#grow-lifecycle-detail"),
  growLastReading: document.querySelector("#grow-last-reading"),
  growReadingStrip: document.querySelector("#grow-reading-strip"),
  growDetailChart: document.querySelector("#grow-detail-chart"),
  growChartTitle: document.querySelector("#grow-chart-title"),
  growDetailRanges: document.querySelector("#grow-detail-ranges"),
  growName: document.querySelector("#grow-name"),
  growStrain: document.querySelector("#grow-strain"),
  growSpecies: document.querySelector("#grow-species"),
  growStage: document.querySelector("#grow-stage"),
  growStbDate: document.querySelector("#grow-stb-date"),
  growSpawnRatio: document.querySelector("#grow-spawn-ratio"),
  growCompletedDate: document.querySelector("#grow-completed-date"),
  growNotes: document.querySelector("#grow-notes"),
  growActive: document.querySelector("#grow-active"),
  growPinDate: document.querySelector("#grow-pin-date"),
  growPinList: document.querySelector("#grow-pin-list"),
  growPhotoCount: document.querySelector("#grow-photo-count"),
  growPhotoFile: document.querySelector("#grow-photo-file"),
  growPhotoDate: document.querySelector("#grow-photo-date"),
  growPhotoCaption: document.querySelector("#grow-photo-caption"),
  growUploadPhoto: document.querySelector("#grow-upload-photo"),
  growPhotoQueue: document.querySelector("#grow-photo-queue"),
  growPhotoGallery: document.querySelector("#grow-photo-gallery"),
  growError: document.querySelector("#grow-error"),
  growSave: document.querySelector("#grow-save"),
  growArchive: document.querySelector("#grow-archive"),
  growContaminated: document.querySelector("#grow-contaminated"),
  growDelete: document.querySelector("#grow-delete"),
  growSpawnHistory: document.querySelector("#grow-spawn-history"),
  growSpawnSummary: document.querySelector("#grow-spawn-summary"),
  growSpawnContent: document.querySelector("#grow-spawn-content"),
  jarDialog: document.querySelector("#jar-dialog"),
  jarForm: document.querySelector("#jar-form"),
  jarDialogTitle: document.querySelector("#jar-dialog-title"),
  jarDialogStatus: document.querySelector("#jar-dialog-status"),
  jarLockedBanner: document.querySelector("#jar-locked-banner"),
  jarLockedTitle: document.querySelector("#jar-locked-title"),
  jarLockedDetail: document.querySelector("#jar-locked-detail"),
  jarName: document.querySelector("#jar-name"),
  jarCulture: document.querySelector("#jar-culture"),
  jarSpecies: document.querySelector("#jar-species"),
  jarGrain: document.querySelector("#jar-grain"),
  jarCountInput: document.querySelector("#jar-count-input"),
  jarQuantityLabel: document.querySelector("#jar-quantity-label"),
  jarDryGrain: document.querySelector("#jar-dry-grain"),
  jarPcTime: document.querySelector("#jar-pc-time"),
  jarPcPsi: document.querySelector("#jar-pc-psi"),
  jarPcDate: document.querySelector("#jar-pc-date"),
  jarInocDate: document.querySelector("#jar-inoc-date"),
  jarPrepTek: document.querySelector("#jar-prep-tek"),
  jarNotes: document.querySelector("#jar-notes"),
  jarBsDate: document.querySelector("#jar-bs-date"),
  jarBsList: document.querySelector("#jar-bs-list"),
  jarPhotoCount: document.querySelector("#jar-photo-count"),
  jarPhotoFile: document.querySelector("#jar-photo-file"),
  jarPhotoDate: document.querySelector("#jar-photo-date"),
  jarPhotoCaption: document.querySelector("#jar-photo-caption"),
  jarUploadPhoto: document.querySelector("#jar-upload-photo"),
  jarPhotoQueue: document.querySelector("#jar-photo-queue"),
  jarPhotoGallery: document.querySelector("#jar-photo-gallery"),
  jarError: document.querySelector("#jar-error"),
  jarSave: document.querySelector("#jar-save"),
  jarSpawn: document.querySelector("#jar-spawn"),
  jarLockToggle: document.querySelector("#jar-lock-toggle"),
  jarArchive: document.querySelector("#jar-archive"),
  jarContaminated: document.querySelector("#jar-contaminated"),
  jarDelete: document.querySelector("#jar-delete"),
  spawnDialog: document.querySelector("#spawn-dialog"),
  spawnForm: document.querySelector("#spawn-form"),
  spawnSourceName: document.querySelector("#spawn-source-name"),
  spawnGroups: document.querySelector("#spawn-groups"),
  spawnError: document.querySelector("#spawn-error"),
  spawnConfirm: document.querySelector("#spawn-confirm"),
  lifecycleDialog: document.querySelector("#lifecycle-dialog"),
  lifecycleForm: document.querySelector("#lifecycle-form"),
  lifecycleTitle: document.querySelector("#lifecycle-title"),
  lifecycleSubtitle: document.querySelector("#lifecycle-subtitle"),
  lifecycleExplanation: document.querySelector("#lifecycle-explanation"),
  lifecycleDateLabel: document.querySelector("#lifecycle-date-label"),
  lifecycleDate: document.querySelector("#lifecycle-date"),
  lifecycleFlushLabel: document.querySelector("#lifecycle-flush-label"),
  lifecycleFirstFlush: document.querySelector("#lifecycle-first-flush"),
  lifecycleReason: document.querySelector("#lifecycle-reason"),
  lifecycleError: document.querySelector("#lifecycle-error"),
  lifecycleConfirm: document.querySelector("#lifecycle-confirm"),
  toast: document.querySelector("#toast"),
};

let cachedNodes = [];
let cachedGrows = [];
let cachedJars = [];
let cachedArchive = { past_grows: [], failed_grows: [], archived_jars: [], failed_jars: [] };
let editingNodeId = null;
let editingGrow = null;
let editingPinDates = [];
let toastTimer = null;
let queuedPhotos = [];
let editingJar = null;
let editingBreakShakeDates = [];
let queuedJarPhotos = [];
let selectedJarIds = new Set();
let spawningJarIds = [];
let lifecycleAction = null;
const chartScaleCache = new Map();
const growRangeHours = new Map();
const GROW_RANGES = [
  { hours: 1, button: "1h", title: "Last hour" },
  { hours: 24, button: "1d", title: "Last day" },
  { hours: 72, button: "3d", title: "Last 3 days" },
  { hours: 168, button: "7d", title: "Last 7 days" },
  { hours: 720, button: "1m", title: "Last month" },
];

function rangeForGrow(tubId) {
  return growRangeHours.get(Number(tubId)) || 72;
}

function rangeDefinition(hours) {
  return GROW_RANGES.find((range) => range.hours === Number(hours)) || GROW_RANGES[2];
}

function showView(name) {
  const title = name === "grows" ? "Tubs & jars" : name.charAt(0).toUpperCase() + name.slice(1);
  ui.navItems.forEach((item) => item.classList.toggle("active", item.dataset.view === name));
  ui.views.forEach((view) => view.classList.toggle("active", view.dataset.viewPanel === name));
  ui.viewTitle.textContent = title;
  ui.sidebar.classList.remove("open");
}

function formatReading(value, suffix, digits = 0) {
  return value === null || value === undefined ? "—" : `${Number(value).toFixed(digits)}${suffix}`;
}

function formatBattery(value) {
  if (value === null || value === undefined) return "—";
  const volts = Number(value);
  if (!Number.isFinite(volts)) return "—";
  return volts < 1 ? `${Math.round(volts * 1000)} mV` : `${volts.toFixed(2)} V`;
}

const BATTERY_THRESHOLDS = Object.freeze({
  dead: 3.35,
  critical: 3.50,
  low: 3.70,
  medium: 4.00,
  full: 4.20,
});

function batteryState(value) {
  const volts = Number(value);
  if (!Number.isFinite(volts)) return "unknown";
  if (volts <= BATTERY_THRESHOLDS.dead) return "dead";
  if (volts <= BATTERY_THRESHOLDS.critical) return "critical";
  if (volts <= BATTERY_THRESHOLDS.low) return "low";
  if (volts < BATTERY_THRESHOLDS.full) return "medium";
  return "full";
}

function batteryStateLabel(state) {
  return ({ dead: "Dead", critical: "Critical", low: "Low", medium: "Medium", full: "Full" })[state] || "Unknown";
}

function makeReading(value, label) {
  const container = document.createElement("div");
  const strong = document.createElement("strong");
  const caption = document.createElement("span");
  strong.textContent = value;
  caption.textContent = label;
  container.append(strong, caption);
  return container;
}

function formatDate(value) {
  if (!value) return "Not set";
  const parsed = new Date(/^\d{4}-\d{2}-\d{2}$/.test(value) ? `${value}T12:00:00` : value);
  return Number.isNaN(parsed.valueOf()) ? value : parsed.toLocaleDateString();
}

function localIsoDate() {
  const now = new Date();
  const month = String(now.getMonth() + 1).padStart(2, "0");
  const day = String(now.getDate()).padStart(2, "0");
  return `${now.getFullYear()}-${month}-${day}`;
}

function formatCaptureTime(photo) {
  if (photo.taken_at_utc) return new Date(photo.taken_at_utc).toLocaleString();
  return photo.taken_on ? formatDate(photo.taken_on) : "Date unavailable";
}

function captureSourceLabel(source) {
  return {
    exif: "photo metadata",
    manual: "manual override",
    manual_date: "manual date",
    file_modified: "file timestamp",
    upload_time: "upload time",
  }[source] || "unknown source";
}

function formatTimeDelta(seconds) {
  if (seconds === null || seconds === undefined) return "";
  const value = Number(seconds);
  if (value < 60) return `${Math.round(value)} sec`;
  if (value < 3600) return `${Math.round(value / 60)} min`;
  return `${(value / 3600).toFixed(1)} hr`;
}

function parseRecordedTime(value) {
  const text = String(value || "");
  return new Date(text.includes("T") ? text : `${text.replace(" ", "T")}Z`);
}

const CHART_METRICS = [
  { key: "temperature_c", name: "Temp", className: "temperature-line", unit: "°C", minimumSpan: 5, step: 1 },
  { key: "humidity_percent", name: "RH", className: "humidity-line", unit: "%", minimumSpan: 20, step: 5, floor: 0, ceiling: 100 },
  { key: "co2_ppm", name: "CO₂", className: "co2-line", unit: " ppm", minimumSpan: 1000, step: 250, floor: 0 },
];

function candidateScale(values, metric) {
  let minimum = Math.min(...values);
  let maximum = Math.max(...values);
  const center = (minimum + maximum) / 2;
  const paddedSpan = Math.max((maximum - minimum) * 1.2, metric.minimumSpan);
  minimum = Math.floor((center - paddedSpan / 2) / metric.step) * metric.step;
  maximum = Math.ceil((center + paddedSpan / 2) / metric.step) * metric.step;
  if (metric.floor !== undefined) minimum = Math.max(metric.floor, minimum);
  if (metric.ceiling !== undefined) maximum = Math.min(metric.ceiling, maximum);
  if (maximum <= minimum) maximum = minimum + metric.minimumSpan;
  return { minimum, maximum };
}

function stableChartScales(history, cacheKey) {
  const cached = chartScaleCache.get(cacheKey) || {};
  const scales = {};
  CHART_METRICS.forEach((metric) => {
    const values = history.map((item) => Number(item[metric.key])).filter(Number.isFinite);
    if (values.length === 0) return;
    const candidate = candidateScale(values, metric);
    const previous = cached[metric.key];
    scales[metric.key] = previous ? {
      minimum: Math.min(previous.minimum, candidate.minimum),
      maximum: Math.max(previous.maximum, candidate.maximum),
    } : candidate;
  });
  chartScaleCache.set(cacheKey, scales);
  return scales;
}

function formatScaleValue(value, metric) {
  if (metric.key === "temperature_c") return Number(value).toFixed(0);
  return Math.round(value).toLocaleString();
}

function chartY(value, scale, bandTop, bandHeight, bandPadding) {
  const usableHeight = Math.max(bandHeight - (bandPadding * 2), 1);
  return bandTop + bandPadding
    + ((scale.maximum - value) / (scale.maximum - scale.minimum) * usableHeight);
}

function chartPath(history, key, scale, width, bandTop, bandHeight, xPadding, bandPadding) {
  const points = history
    .map((item, index) => ({ index, value: Number(item[key]) }))
    .filter((item) => Number.isFinite(item.value));
  if (points.length === 0) return "";
  const usableWidth = width - (xPadding * 2);
  const denominator = Math.max(history.length - 1, 1);
  return points.map((point, pointIndex) => {
    const x = xPadding + ((point.index / denominator) * usableWidth);
    const y = chartY(point.value, scale, bandTop, bandHeight, bandPadding);
    return `${pointIndex === 0 ? "M" : "L"}${x.toFixed(1)},${y.toFixed(1)}`;
  }).join(" ");
}

function renderChart(container, history, rangeTitle = "selected time range", cacheKey = rangeTitle) {
  container.replaceChildren();
  if (!history || history.length === 0) {
    const empty = document.createElement("div");
    empty.className = "grow-chart-empty";
    empty.textContent = "No readings during this grow yet";
    container.append(empty);
    return;
  }
  const namespace = "http://www.w3.org/2000/svg";
  const width = 600;
  const height = 180;
  const svg = document.createElementNS(namespace, "svg");
  svg.setAttribute("viewBox", `0 0 ${width} ${height}`);
  svg.setAttribute("preserveAspectRatio", "none");
  svg.setAttribute("role", "img");
  svg.setAttribute("aria-label", `Temperature, humidity, and CO2 history for ${rangeTitle.toLowerCase()}`);
  const scales = stableChartScales(history, cacheKey);
  const activeMetrics = CHART_METRICS.filter((metric) => scales[metric.key]);
  const topPadding = 8;
  const bottomPadding = 18;
  const xPadding = 9;
  const bandPadding = 5;
  const bandGap = 8;
  const bandHeight = (
    height - topPadding - bottomPadding
      - (bandGap * Math.max(activeMetrics.length - 1, 0))
  ) / Math.max(activeMetrics.length, 1);
  const averages = {};
  const bandTops = {};

  activeMetrics.forEach((metric, metricIndex) => {
    const scale = scales[metric.key];
    const values = history
      .map((item) => Number(item[metric.key]))
      .filter(Number.isFinite);
    const average = values.reduce((total, value) => total + value, 0) / values.length;
    const bandTop = topPadding + (metricIndex * (bandHeight + bandGap));
    averages[metric.key] = average;
    bandTops[metric.key] = bandTop;

    if (metricIndex > 0) {
      const separator = document.createElementNS(namespace, "line");
      const separatorY = bandTop - (bandGap / 2);
      separator.setAttribute("x1", "0");
      separator.setAttribute("x2", String(width));
      separator.setAttribute("y1", String(separatorY));
      separator.setAttribute("y2", String(separatorY));
      separator.setAttribute("class", "chart-band-separator");
      svg.append(separator);
    }

    const averageLine = document.createElementNS(namespace, "line");
    const averageY = chartY(average, scale, bandTop, bandHeight, bandPadding);
    averageLine.setAttribute("x1", String(xPadding));
    averageLine.setAttribute("x2", String(width - xPadding));
    averageLine.setAttribute("y1", String(averageY));
    averageLine.setAttribute("y2", String(averageY));
    averageLine.setAttribute(
      "class",
      `chart-average-line ${metric.className.replace("-line", "-average")}`,
    );
    svg.append(averageLine);

    const pathData = chartPath(
      history,
      metric.key,
      scale,
      width,
      bandTop,
      bandHeight,
      xPadding,
      bandPadding,
    );
    if (!pathData) return;
    const path = document.createElementNS(namespace, "path");
    path.setAttribute("d", pathData);
    path.setAttribute("class", metric.className);
    svg.append(path);
  });
  container.append(svg);

  const scaleKey = document.createElement("div");
  scaleKey.className = "chart-scale-key";
  activeMetrics.forEach((metric) => {
    const scale = scales[metric.key];
    const row = document.createElement("span");
    row.className = metric.className.replace("-line", "");
    row.style.top = `${((bandTops[metric.key] + bandPadding) / height) * 100}%`;
    row.textContent = `${metric.name} ${formatScaleValue(scale.minimum, metric)} / avg ${formatScaleValue(averages[metric.key], metric)} / ${formatScaleValue(scale.maximum, metric)}${metric.unit}`;
    scaleKey.append(row);
  });
  container.append(scaleKey);

  const timeScale = document.createElement("div");
  timeScale.className = "chart-time-scale";
  const firstTime = parseRecordedTime(history[0].recorded_utc);
  const lastTime = parseRecordedTime(history[history.length - 1].recorded_utc);
  timeScale.innerHTML = `<span>${firstTime.toLocaleString([], { month: "short", day: "numeric", hour: "numeric", minute: "2-digit" })}</span><span>${lastTime.toLocaleString([], { month: "short", day: "numeric", hour: "numeric", minute: "2-digit" })}</span>`;
  container.append(timeScale);
}

function makeGrowRangeSelector(tubId, activeHours) {
  const selector = document.createElement("div");
  selector.className = "grow-range-selector";
  selector.setAttribute("aria-label", "Graph time range");
  selector.addEventListener("click", (event) => event.stopPropagation());
  selector.addEventListener("keydown", (event) => event.stopPropagation());
  const label = document.createElement("span");
  label.className = "grow-range-label";
  label.textContent = "Graph range";
  selector.append(label);
  GROW_RANGES.forEach((range) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `grow-range-button${range.hours === activeHours ? " active" : ""}`;
    button.textContent = range.button;
    button.title = range.title;
    button.setAttribute("aria-label", `Show ${range.title.toLowerCase()}`);
    button.setAttribute("aria-pressed", String(range.hours === activeHours));
    button.addEventListener("click", () => selectGrowRange(tubId, range.hours));
    selector.append(button);
  });
  return selector;
}

function makeGrowReading(value, label, className = "") {
  const box = document.createElement("div");
  box.className = `grow-card-reading ${className}`.trim();
  const strong = document.createElement("strong");
  const span = document.createElement("span");
  strong.textContent = value;
  span.textContent = label;
  box.append(strong, span);
  return box;
}

function makeGrowCard(grow) {
  const card = document.createElement("article");
  card.className = `grow-card${grow.archive_category ? " archived-card" : ""}${grow.archive_category === "failed_grow" ? " failed-card" : ""}`;
  card.tabIndex = 0;
  card.setAttribute("role", "button");
  card.setAttribute("aria-label", `Open ${grow.title}`);

  const header = document.createElement("div");
  header.className = "grow-card-header";
  const title = document.createElement("div");
  title.className = "grow-card-title";
  const titleText = document.createElement("strong");
  titleText.textContent = grow.title;
  const nodeText = document.createElement("span");
  nodeText.textContent = grow.node_id
    ? `Node ${grow.node_id} · ${grow.name}`
    : `No sensor assigned · ${grow.name}`;
  title.append(titleText, nodeText);
  const stage = document.createElement("span");
  stage.className = `stage-badge ${grow.stage}`;
  stage.textContent = grow.archive_category === "failed_grow"
    ? "failed"
    : grow.archive_category === "past_grow"
      ? (grow.contaminated_on ? "contaminated after flush" : "finished")
      : grow.stage;
  header.append(title);
  const battery = batteryState(grow.battery_voltage_v);
  if (["low", "critical", "dead"].includes(battery)) {
    const alert = document.createElement("span");
    alert.className = `battery-alert ${battery}`;
    alert.textContent = "!";
    alert.title = `Node battery ${batteryStateLabel(battery).toLowerCase()} (${formatBattery(grow.battery_voltage_v)})`;
    header.append(alert);
  }
  header.append(stage);

  const activeHours = rangeForGrow(grow.tub_id);
  const range = rangeDefinition(activeHours);
  const rangeSelector = makeGrowRangeSelector(grow.tub_id, activeHours);

  const chart = document.createElement("div");
  chart.className = "grow-chart";
  renderChart(chart, grow.history, range.title, `${grow.tub_id}:${activeHours}`);
  const legend = document.createElement("div");
  legend.className = "chart-legend";
  legend.innerHTML = '<span class="temperature">Temp</span><span class="humidity">Humidity</span><span class="co2">CO₂</span><span class="average-guide">Dotted = average</span>';

  const readings = document.createElement("div");
  readings.className = "grow-card-readings";
  readings.append(
    makeGrowReading(formatReading(grow.temperature_c, " °C", 1), "Temperature"),
    makeGrowReading(formatReading(grow.humidity_percent, "%", 1), "Humidity"),
    makeGrowReading(formatBattery(grow.battery_voltage_v), `Battery · ${batteryStateLabel(battery)}`, `battery-${battery}`),
    makeGrowReading(formatReading(grow.co2_ppm, " ppm"), "CO₂"),
  );

  const footer = document.createElement("div");
  footer.className = "grow-card-footer";
  const stb = document.createElement("span");
  stb.textContent = `STB: ${formatDate(grow.spawn_to_bulk_on)}`;
  const photos = document.createElement("span");
  photos.textContent = `${grow.photo_count} photo${grow.photo_count === 1 ? "" : "s"}`;
  footer.append(stb);
  if (grow.archive_category) {
    const outcome = document.createElement("span");
    outcome.textContent = grow.contaminated_on
      ? `Contaminated: ${formatDate(grow.contaminated_on)}`
      : `Finished: ${formatDate(grow.completed_on)}`;
    outcome.title = grow.lifecycle_reason || "";
    footer.append(outcome);
  }
  footer.append(photos);
  card.append(header, rangeSelector, chart, legend, readings, footer);

  const open = () => openGrow(grow.tub_id);
  card.addEventListener("click", open);
  card.addEventListener("keydown", (event) => {
    if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      open();
    }
  });
  return card;
}

function renderGrows() {
  ui.growGrid.replaceChildren(...cachedGrows.map(makeGrowCard));
  ui.growGrid.hidden = cachedGrows.length === 0;
  ui.growEmpty.hidden = cachedGrows.length !== 0;
  ui.growCount.textContent = cachedGrows.length.toLocaleString();
}

function makeJarFact(value, label) {
  const fact = document.createElement("div");
  const strong = document.createElement("strong");
  const caption = document.createElement("span");
  strong.textContent = value || "Not set";
  caption.textContent = label;
  fact.append(strong, caption);
  return fact;
}

function makeJarCard(jar, { selectable = true } = {}) {
  const card = document.createElement("article");
  card.className = `grow-card jar-card${selectedJarIds.has(jar.jar_id) ? " selected" : ""}${jar.archive_category ? " archived-card" : ""}${jar.archive_category === "failed_jar" ? " failed-card" : ""}`;
  card.tabIndex = 0;
  card.setAttribute("role", "button");
  card.setAttribute("aria-label", `Open ${jar.name}`);
  const header = document.createElement("div");
  header.className = "grow-card-header";
  const title = document.createElement("div");
  title.className = "grow-card-title";
  const name = document.createElement("strong");
  name.textContent = jar.name;
  const subtitle = document.createElement("span");
  subtitle.textContent = [jar.culture || "Culture not set", jar.species].filter(Boolean).join(" · ");
  title.append(name, subtitle);
  header.append(title);
  if (jar.archive_category) {
    const outcome = document.createElement("span");
    outcome.className = `stage-badge${jar.archive_category === "failed_jar" ? " failed" : " complete"}`;
    outcome.textContent = jar.archive_category === "failed_jar" ? "failed" : "archived";
    header.append(outcome);
  }
  let selection = null;
  if (selectable) {
    selection = document.createElement("label");
    selection.className = "jar-select";
    selection.title = `Select ${jar.name} for spawning`;
    const checkbox = document.createElement("input");
    checkbox.type = "checkbox";
    checkbox.checked = selectedJarIds.has(jar.jar_id);
    checkbox.setAttribute("aria-label", `Select ${jar.name}`);
    checkbox.addEventListener("click", (event) => event.stopPropagation());
    checkbox.addEventListener("change", () => {
      if (checkbox.checked) selectedJarIds.add(jar.jar_id);
      else selectedJarIds.delete(jar.jar_id);
      card.classList.toggle("selected", checkbox.checked);
      updateJarSelectionControls();
    });
    selection.addEventListener("click", (event) => event.stopPropagation());
    selection.append(checkbox);
  }
  const facts = document.createElement("div");
  facts.className = "jar-card-body";
  facts.append(
    makeJarFact(jar.grain_type, "Grain"),
    makeJarFact(`Jar ${jar.jar_id}`, "Database ID"),
    makeJarFact(formatDate(jar.inoculated_on), "Inoculated"),
    makeJarFact(
      jar.break_shake_dates.length
        ? formatDate(jar.break_shake_dates[jar.break_shake_dates.length - 1])
        : "None yet",
      "Latest B/S",
    ),
  );
  const note = document.createElement("div");
  note.className = "jar-card-note";
  note.textContent = jar.lifecycle_reason || jar.notes || jar.prep_tek || "No notes recorded.";
  const footer = document.createElement("div");
  footer.className = "grow-card-footer";
  const weight = document.createElement("span");
  weight.textContent = jar.dry_grain_grams_per_jar == null
    ? "Dry weight not set" : `${jar.dry_grain_grams_per_jar} g dry / jar`;
  const photos = document.createElement("span");
  photos.textContent = `${jar.photo_count} photo${jar.photo_count === 1 ? "" : "s"}`;
  footer.append(weight, photos);
  card.append(...[selection, header, facts, note, footer].filter(Boolean));
  const open = () => openJar(jar.jar_id);
  card.addEventListener("click", open);
  card.addEventListener("keydown", (event) => {
    if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      open();
    }
  });
  return card;
}

function updateJarSelectionControls() {
  const count = selectedJarIds.size;
  ui.spawnSelectedJars.disabled = count === 0;
  ui.spawnSelectedJars.textContent = count
    ? `Spawn selected (${count})`
    : "Spawn selected";
}

function renderJars() {
  ui.jarGrid.replaceChildren(...cachedJars.map(makeJarCard));
  ui.jarGrid.hidden = cachedJars.length === 0;
  ui.jarEmpty.hidden = cachedJars.length !== 0;
  ui.jarCount.textContent = cachedJars.length.toLocaleString();
  updateJarSelectionControls();
}

async function refreshJars() {
  try {
    const response = await fetch("/api/jars", { cache: "no-store" });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    cachedJars = data.jars;
    const currentIds = new Set(cachedJars.map((jar) => jar.jar_id));
    selectedJarIds = new Set([...selectedJarIds].filter((id) => currentIds.has(id)));
    renderJars();
  } catch (error) {
    console.error("Could not load current jars", error);
  }
}

async function refreshGrows() {
  try {
    const response = await fetch("/api/grows?hours=72", { cache: "no-store" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const data = await response.json();
    cachedGrows = await Promise.all(data.grows.map(async (grow) => {
      const hours = rangeForGrow(grow.tub_id);
      if (hours === 72) return grow;
      const customResponse = await fetch(`/api/grows/${grow.tub_id}?hours=${hours}`, { cache: "no-store" });
      if (!customResponse.ok) return grow;
      const customData = await customResponse.json();
      return customData.grow;
    }));
    renderGrows();
  } catch (error) {
    console.error("Could not load current grows", error);
  }
}

function renderArchiveCollection(grid, empty, count, records, makeCard) {
  grid.replaceChildren(...records.map(makeCard));
  grid.hidden = records.length === 0;
  empty.hidden = records.length !== 0;
  count.textContent = records.length.toLocaleString();
}

function renderArchive() {
  renderArchiveCollection(
    ui.pastGrowGrid, ui.pastGrowEmpty, ui.pastGrowCount,
    cachedArchive.past_grows, makeGrowCard,
  );
  renderArchiveCollection(
    ui.failedGrowGrid, ui.failedGrowEmpty, ui.failedGrowCount,
    cachedArchive.failed_grows, makeGrowCard,
  );
  renderArchiveCollection(
    ui.archivedJarGrid, ui.archivedJarEmpty, ui.archivedJarCount,
    cachedArchive.archived_jars, (jar) => makeJarCard(jar, { selectable: false }),
  );
  renderArchiveCollection(
    ui.failedJarGrid, ui.failedJarEmpty, ui.failedJarCount,
    cachedArchive.failed_jars, (jar) => makeJarCard(jar, { selectable: false }),
  );
}

async function refreshArchive() {
  try {
    const response = await fetch("/api/archive?hours=72", { cache: "no-store" });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    const loadCustomRanges = (records) => Promise.all(records.map(async (grow) => {
      const hours = rangeForGrow(grow.tub_id);
      if (hours === 72) return grow;
      const customResponse = await fetch(`/api/grows/${grow.tub_id}?hours=${hours}`, { cache: "no-store" });
      if (!customResponse.ok) return grow;
      return (await customResponse.json()).grow;
    }));
    data.past_grows = await loadCustomRanges(data.past_grows);
    data.failed_grows = await loadCustomRanges(data.failed_grows);
    cachedArchive = data;
    renderArchive();
  } catch (error) {
    console.error("Could not load archive", error);
  }
}

async function selectGrowRange(tubId, hours) {
  growRangeHours.set(Number(tubId), Number(hours));
  try {
    const response = await fetch(`/api/grows/${tubId}?hours=${hours}`, { cache: "no-store" });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    const index = cachedGrows.findIndex((grow) => grow.tub_id === Number(tubId));
    if (index >= 0) cachedGrows[index] = data.grow;
    [cachedArchive.past_grows, cachedArchive.failed_grows].forEach((records) => {
      const archiveIndex = records.findIndex((grow) => grow.tub_id === Number(tubId));
      if (archiveIndex >= 0) records[archiveIndex] = data.grow;
    });
    renderGrows();
    renderArchive();
    if (ui.growDialog.open && editingGrow?.tub_id === Number(tubId)) {
      populateGrowDialog(data.grow);
    }
  } catch (error) {
    showToast(error.message || "Could not change graph range.");
  }
}

function makeNodeRow(node) {
  const row = document.createElement("article");
  row.className = "node-row";

  const identity = document.createElement("div");
  identity.className = "node-identity";
  const name = document.createElement("strong");
  name.textContent = node.name;
  const detail = document.createElement("span");
  detail.textContent = `Node ${node.node_id} · FW ${node.firmware_version || "unknown"} · ${node.tub_name || "Unassigned"} · ${new Date(node.last_seen_utc).toLocaleString()}`;
  const configState = document.createElement("span");
  configState.className = `node-config-state ${node.command_status}`;
  configState.textContent = `Config: ${node.command_status}`;
  identity.append(name, detail, configState);

  const settingsButton = document.createElement("button");
  settingsButton.className = "node-settings-button";
  settingsButton.type = "button";
  settingsButton.title = `Configure ${node.name}`;
  settingsButton.setAttribute("aria-label", `Configure ${node.name}`);
  settingsButton.textContent = "⚙";
  settingsButton.addEventListener("click", () => openNodeSettings(node.node_id));

  row.append(
    identity,
    makeReading(formatReading(node.co2_ppm, " ppm"), "CO₂"),
    makeReading(formatReading(node.temperature_c, " °C", 1), "Temperature"),
    makeReading(formatReading(node.humidity_percent, "%", 1), "Humidity"),
    (() => {
      const reading = makeReading(formatBattery(node.battery_voltage_v), `Battery · ${batteryStateLabel(batteryState(node.battery_voltage_v))}`);
      reading.className = `battery-reading battery-${batteryState(node.battery_voltage_v)}`;
      return reading;
    })(),
    makeReading(formatReading(node.rssi_dbm, " dBm", 1), "Signal"),
    settingsButton,
  );
  return row;
}

function showToast(message) {
  window.clearTimeout(toastTimer);
  ui.toast.textContent = message;
  ui.toast.hidden = false;
  toastTimer = window.setTimeout(() => { ui.toast.hidden = true; }, 3500);
}

async function loadTubOptions() {
  try {
    const response = await fetch("/api/tubs", { cache: "no-store" });
    if (!response.ok) return;
    const data = await response.json();
    ui.tubOptions.replaceChildren(...data.tubs.map((tub) => {
      const option = document.createElement("option");
      option.value = tub.name;
      return option;
    }));
  } catch (error) {
    console.warn("Could not load tub names", error);
  }
}

async function openNodeSettings(nodeId) {
  const node = cachedNodes.find((item) => item.node_id === nodeId);
  if (!node) return;
  editingNodeId = nodeId;
  ui.settingsNodeId.textContent = `Permanent node ID: ${nodeId} · Firmware: ${node.firmware_version || "unknown"}`;
  ui.settingsName.value = node.name;
  ui.settingsTub.value = node.tub_name || "";
  ui.settingsLocation.value = node.location || "";
  ui.settingsNotes.value = node.notes || "";
  ui.settingsActive.checked = node.active;
  ui.settingsInterval.value = node.desired_report_interval_s;
  ui.settingsDownlinkWindow.value = node.desired_downlink_window_ms;
  ui.settingsConfigStatus.textContent = node.command_status;
  ui.settingsConfigStatus.className = `config-badge ${node.command_status}`;
  ui.settingsError.hidden = true;
  await loadTubOptions();
  ui.settingsDialog.showModal();
}

async function saveNodeSettings(event) {
  event.preventDefault();
  if (editingNodeId === null) return;
  ui.settingsSave.disabled = true;
  ui.settingsError.hidden = true;
  try {
    const response = await fetch(`/api/nodes/${editingNodeId}`, {
      method: "PUT",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        name: ui.settingsName.value.trim(),
        tub_name: ui.settingsTub.value.trim(),
        location: ui.settingsLocation.value.trim(),
        notes: ui.settingsNotes.value.trim(),
        active: ui.settingsActive.checked,
        report_interval_s: Number(ui.settingsInterval.value),
        downlink_window_ms: Number(ui.settingsDownlinkWindow.value),
      }),
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    ui.settingsDialog.close();
    showToast(data.node.command_status === "queued"
      ? "Node saved; device configuration queued."
      : "Node settings saved.");
    await Promise.all([refreshDashboard(), refreshGrows()]);
  } catch (error) {
    ui.settingsError.textContent = error.message || "Could not save node settings.";
    ui.settingsError.hidden = false;
  } finally {
    ui.settingsSave.disabled = false;
  }
}

function renderPinDates() {
  ui.growPinList.replaceChildren();
  if (editingPinDates.length === 0) {
    const empty = document.createElement("span");
    empty.className = "date-chip-empty";
    empty.textContent = "No pin dates recorded.";
    ui.growPinList.append(empty);
    return;
  }
  editingPinDates.forEach((value) => {
    const chip = document.createElement("span");
    chip.className = "date-chip";
    const text = document.createElement("span");
    text.textContent = formatDate(value);
    const remove = document.createElement("button");
    remove.type = "button";
    remove.setAttribute("aria-label", `Remove ${value}`);
    remove.textContent = "×";
    remove.addEventListener("click", () => {
      editingPinDates = editingPinDates.filter((item) => item !== value);
      renderPinDates();
    });
    chip.append(text, remove);
    ui.growPinList.append(chip);
  });
}

function renderGrowPhotos(photos) {
  ui.growPhotoGallery.replaceChildren();
  ui.growPhotoCount.textContent = `${photos.length} photo${photos.length === 1 ? "" : "s"}`;
  if (photos.length === 0) {
    const empty = document.createElement("div");
    empty.className = "photo-empty";
    empty.textContent = "No photos uploaded for this grow.";
    ui.growPhotoGallery.append(empty);
    return;
  }
  photos.forEach((photo) => {
    const card = document.createElement("article");
    card.className = "photo-card";
    const image = document.createElement("img");
    image.src = `/api/grow-photos/${photo.photo_id}`;
    image.alt = photo.caption || photo.original_name;
    image.loading = "lazy";
    const copy = document.createElement("div");
    copy.className = "photo-card-copy";
    const caption = document.createElement("strong");
    caption.textContent = photo.caption || photo.original_name;
    const date = document.createElement("span");
    date.textContent = `${formatCaptureTime(photo)} · ${captureSourceLabel(photo.capture_time_source)}`;
    const conditions = document.createElement("span");
    conditions.className = "photo-conditions";
    const conditionParts = [];
    if (photo.condition_temperature_c !== null && photo.condition_temperature_c !== undefined) {
      conditionParts.push(`${Number(photo.condition_temperature_c).toFixed(1)} °C`);
    }
    if (photo.condition_humidity_percent !== null && photo.condition_humidity_percent !== undefined) {
      conditionParts.push(`${Number(photo.condition_humidity_percent).toFixed(1)}% RH`);
    }
    if (photo.condition_co2_ppm !== null && photo.condition_co2_ppm !== undefined) {
      conditionParts.push(`${photo.condition_co2_ppm} ppm CO₂`);
    }
    if (photo.condition_battery_voltage_v !== null && photo.condition_battery_voltage_v !== undefined) {
      conditionParts.push(formatBattery(photo.condition_battery_voltage_v));
    }
    const delta = formatTimeDelta(photo.condition_time_delta_s);
    conditions.textContent = conditionParts.length
      ? `${conditionParts.join(" · ")} · nearest reading${delta ? ` ${delta} away` : ""}`
      : "No grow measurement was available for this capture time";
    copy.append(caption, date, conditions);
    const remove = document.createElement("button");
    remove.className = "photo-delete";
    remove.type = "button";
    remove.title = "Delete photo";
    remove.setAttribute("aria-label", `Delete ${photo.original_name}`);
    remove.textContent = "×";
    remove.addEventListener("click", () => deleteGrowPhoto(photo.photo_id));
    card.append(image, copy, remove);
    ui.growPhotoGallery.append(card);
  });
}

function clearPhotoQueue() {
  queuedPhotos.forEach((entry) => URL.revokeObjectURL(entry.previewUrl));
  queuedPhotos = [];
  ui.growPhotoFile.value = "";
  renderPhotoQueue();
}

function renderPhotoQueue() {
  ui.growPhotoQueue.replaceChildren();
  ui.growUploadPhoto.textContent = queuedPhotos.length
    ? `Upload ${queuedPhotos.length} photo${queuedPhotos.length === 1 ? "" : "s"}`
    : "Upload queued";
  ui.growUploadPhoto.disabled = queuedPhotos.length === 0;
  if (queuedPhotos.length === 0) {
    const empty = document.createElement("div");
    empty.className = "photo-queue-empty";
    empty.textContent = "No photos queued.";
    ui.growPhotoQueue.append(empty);
    return;
  }
  queuedPhotos.forEach((entry) => {
    const card = document.createElement("div");
    card.className = "photo-queue-card";
    const image = document.createElement("img");
    image.src = entry.previewUrl;
    image.alt = "";
    const details = document.createElement("div");
    const name = document.createElement("strong");
    name.textContent = entry.file.name;
    const size = document.createElement("span");
    size.textContent = `${(entry.file.size / (1024 * 1024)).toFixed(1)} MB`;
    details.append(name, size);
    const remove = document.createElement("button");
    remove.type = "button";
    remove.textContent = "×";
    remove.setAttribute("aria-label", `Remove ${entry.file.name} from queue`);
    remove.addEventListener("click", () => {
      URL.revokeObjectURL(entry.previewUrl);
      queuedPhotos = queuedPhotos.filter((item) => item.id !== entry.id);
      renderPhotoQueue();
    });
    card.append(image, details, remove);
    ui.growPhotoQueue.append(card);
  });
}

function queueSelectedPhotos() {
  const supported = new Set(["image/jpeg", "image/png", "image/webp"]);
  const typeByExtension = new Map([
    ["jpg", "image/jpeg"], ["jpeg", "image/jpeg"],
    ["png", "image/png"], ["webp", "image/webp"],
  ]);
  const existing = new Set(queuedPhotos.map((entry) => entry.id));
  const rejected = [];
  [...ui.growPhotoFile.files].forEach((file) => {
    const id = `${file.name}:${file.size}:${file.lastModified}`;
    if (existing.has(id)) return;
    const extension = file.name.split(".").pop().toLowerCase();
    const normalizedBrowserType = { "image/jpg": "image/jpeg", "image/pjpeg": "image/jpeg" }[file.type] || file.type;
    const mediaType = supported.has(normalizedBrowserType)
      ? normalizedBrowserType
      : (typeByExtension.get(extension) || "");
    if (!supported.has(mediaType) || file.size === 0 || file.size > (10 * 1024 * 1024)) {
      rejected.push(file.name);
      return;
    }
    existing.add(id);
    queuedPhotos.push({ id, file, mediaType, previewUrl: URL.createObjectURL(file) });
  });
  ui.growPhotoFile.value = "";
  renderPhotoQueue();
  if (rejected.length) {
    ui.growError.textContent = `${rejected.join(", ")} could not be queued. Use JPEG, PNG, or WebP files up to 10 MB.`;
    ui.growError.hidden = false;
  }
}

function renderSpawnJarHistory(jars) {
  ui.growSpawnContent.replaceChildren();
  const records = jars || [];
  ui.growSpawnHistory.hidden = records.length === 0;
  if (records.length === 0) return;
  ui.growSpawnHistory.querySelector("summary span:first-child").textContent = records.length === 1 ? "Spawn Jar" : "Spawn Jars";
  ui.growSpawnSummary.textContent = records.length === 1
    ? `${records[0].name} - ${records[0].locked ? "locked history" : "unlocked"}`
    : `${records.length} individual jars - inherited history`;
  const content = document.createElement("div");
  content.className = "spawn-history-content";
  records.forEach((jar) => {
    const record = document.createElement("section");
    record.className = "spawn-jar-record";
    const heading = document.createElement("h4");
    heading.textContent = jar.name;
    record.append(heading);
    const facts = document.createElement("div");
    facts.className = "spawn-facts";
    facts.append(
      makeJarFact(jar.culture, "Culture"),
      makeJarFact(jar.grain_type, "Grain"),
      makeJarFact(jar.dry_grain_grams_per_jar == null ? "Not set" : `${jar.dry_grain_grams_per_jar} g`, "Dry grain"),
      makeJarFact(jar.pressure_cooker_minutes == null ? "Not set" : `${jar.pressure_cooker_minutes} min`, "PC time"),
      makeJarFact(jar.pressure_psi == null ? "Not set" : `${jar.pressure_psi} PSI`, "PC pressure"),
      makeJarFact(formatDate(jar.pressure_cooked_on), "PC date"),
      makeJarFact(formatDate(jar.inoculated_on), "Inoculated"),
      makeJarFact(jar.locked ? "Locked" : "Unlocked", "Record state"),
    );
    record.append(facts);
    const dates = document.createElement("p");
    dates.className = "form-note";
    dates.textContent = `Break & shake: ${jar.break_shake_dates.length ? jar.break_shake_dates.map(formatDate).join(", ") : "None recorded"}`;
    record.append(dates);
    if (jar.prep_tek) {
      const prep = document.createElement("p");
      prep.className = "form-note";
      prep.textContent = `Prep tek: ${jar.prep_tek}`;
      record.append(prep);
    }
    if (jar.notes) {
      const notes = document.createElement("p");
      notes.className = "form-note";
      notes.textContent = `Jar notes: ${jar.notes}`;
      record.append(notes);
    }
    const gallery = document.createElement("div");
    gallery.className = "photo-gallery";
    gallery.append(renderJarPhotos(jar.photos, { historical: true }));
    record.append(gallery);
    const actions = document.createElement("div");
    actions.className = "spawn-history-actions";
    const button = document.createElement("button");
    button.className = jar.locked ? "button caution" : "button";
    button.type = "button";
    button.textContent = jar.locked ? "Unlock this jar record" : "Edit this jar record";
    button.addEventListener("click", async () => {
      if (jar.locked) {
        if (!window.confirm(`Unlock ${jar.name}? Only do this to correct old data.`)) return;
        try {
          const response = await fetch(`/api/jars/${jar.jar_id}/lock`, {
            method: "PUT", headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ locked: false }),
          });
          const data = await response.json();
          if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
          editingGrow.spawn_jars = editingGrow.spawn_jars.map((item) => item.jar_id === jar.jar_id ? data.jar : item);
          editingGrow.spawn_jar = editingGrow.spawn_jars[0] || null;
          renderSpawnJarHistory(editingGrow.spawn_jars);
          showToast(`${jar.name} unlocked for correction.`);
        } catch (error) {
          ui.growError.textContent = error.message || "Could not unlock spawn record.";
          ui.growError.hidden = false;
        }
      } else {
        ui.growDialog.close();
        await openJar(jar.jar_id);
      }
    });
    actions.append(button);
    record.append(actions);
    content.append(record);
  });
  ui.growSpawnContent.append(content);
}

function populateGrowDialog(grow) {
  const openingGrow = !ui.growDialog.open || editingGrow?.tub_id !== grow.tub_id;
  if (openingGrow) clearPhotoQueue();
  editingGrow = grow;
  editingPinDates = [...grow.pin_dates];
  ui.growDialogTitle.textContent = grow.title;
  ui.growDialogNode.textContent = `Node ${grow.node_id} · permanent node ID`;
  if (!grow.node_id) ui.growDialogNode.textContent = "No sensor assigned";
  ui.growLastReading.textContent = grow.reading_utc
    ? `Last reading ${new Date(grow.reading_utc).toLocaleString()}`
    : "No readings";
  const activeHours = rangeForGrow(grow.tub_id);
  const range = rangeDefinition(activeHours);
  ui.growChartTitle.textContent = range.title;
  const detailRanges = makeGrowRangeSelector(grow.tub_id, activeHours);
  ui.growDetailRanges.replaceChildren(...detailRanges.children);
  ui.growReadingStrip.replaceChildren(
    makeGrowReading(formatReading(grow.temperature_c, " °C", 1), "Temperature"),
    makeGrowReading(formatReading(grow.humidity_percent, "%", 1), "Humidity"),
    makeGrowReading(formatBattery(grow.battery_voltage_v), "Battery"),
    makeGrowReading(formatReading(grow.co2_ppm, " ppm"), "CO₂"),
    makeGrowReading(formatReading(grow.rssi_dbm, " dBm", 1), "Signal"),
  );
  renderChart(ui.growDetailChart, grow.history, range.title, `${grow.tub_id}:${activeHours}`);
  ui.growName.value = grow.name;
  ui.growStrain.value = grow.strain || "";
  ui.growSpecies.value = grow.species || "";
  ui.growStage.value = grow.stage;
  ui.growStbDate.value = grow.spawn_to_bulk_on || "";
  ui.growSpawnRatio.value = grow.spawn_ratio || "";
  ui.growCompletedDate.value = grow.completed_on || "";
  ui.growNotes.value = grow.notes || "";
  ui.growActive.checked = grow.active;
  ui.growActive.disabled = Boolean(grow.archive_category);
  ui.growLifecycleBanner.hidden = !grow.archive_category;
  ui.growLifecycleBanner.classList.toggle("failed", grow.archive_category === "failed_grow");
  ui.growLifecycleTitle.textContent = grow.archive_category === "failed_grow"
    ? "Failed grow · contaminated before first flush"
    : grow.contaminated_on
      ? "Past grow · contaminated after a flush"
      : "Past grow · finished";
  ui.growLifecycleDetail.textContent = [
    grow.contaminated_on
      ? `Contaminated ${formatDate(grow.contaminated_on)}`
      : `Finished ${formatDate(grow.completed_on)}`,
    grow.lifecycle_reason,
  ].filter(Boolean).join(" · ");
  ui.growArchive.hidden = !grow.active || Boolean(grow.archive_category);
  ui.growContaminated.hidden = !grow.active || Boolean(grow.archive_category);
  ui.growDelete.hidden = false;
  ui.growPinDate.value = "";
  if (openingGrow) {
    ui.growPhotoDate.value = "";
    ui.growPhotoCaption.value = "";
  }
  ui.growError.hidden = true;
  renderPinDates();
  renderGrowPhotos(grow.photos);
  renderSpawnJarHistory(grow.spawn_jars || (grow.spawn_jar ? [grow.spawn_jar] : []));
}

async function openGrow(tubId) {
  try {
    const response = await fetch(`/api/grows/${tubId}?hours=${rangeForGrow(tubId)}`, { cache: "no-store" });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    populateGrowDialog(data.grow);
    ui.growDialog.showModal();
  } catch (error) {
    showToast(error.message || "Could not open grow.");
  }
}

function addPinDate() {
  const value = ui.growPinDate.value;
  if (!value || editingPinDates.includes(value)) return;
  editingPinDates.push(value);
  editingPinDates.sort();
  ui.growPinDate.value = "";
  renderPinDates();
}

async function saveGrow(event) {
  event.preventDefault();
  if (!editingGrow) return;
  ui.growSave.disabled = true;
  ui.growError.hidden = true;
  try {
    const response = await fetch(`/api/grows/${editingGrow.tub_id}`, {
      method: "PUT",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        name: ui.growName.value.trim(),
        species: ui.growSpecies.value.trim(),
        strain: ui.growStrain.value.trim(),
        stage: ui.growStage.value,
        spawn_to_bulk_on: ui.growStbDate.value || null,
        spawn_ratio: ui.growSpawnRatio.value.trim(),
        completed_on: ui.growCompletedDate.value || null,
        pin_dates: editingPinDates,
        notes: ui.growNotes.value.trim(),
        active: ui.growActive.checked,
      }),
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    ui.growDialog.close();
    showToast("Grow saved.");
    await Promise.all([refreshDashboard(), refreshGrows(), refreshArchive(), loadTubOptions()]);
  } catch (error) {
    ui.growError.textContent = error.message || "Could not save grow.";
    ui.growError.hidden = false;
  } finally {
    ui.growSave.disabled = false;
  }
}

async function uploadGrowPhoto() {
  if (!editingGrow) return;
  if (queuedPhotos.length === 0) {
    ui.growError.textContent = "Add one or more JPEG, PNG, or WebP images to the queue first.";
    ui.growError.hidden = false;
    return;
  }
  ui.growUploadPhoto.disabled = true;
  ui.growError.hidden = true;
  const pending = [...queuedPhotos];
  const failed = [];
  const uploaded = [];
  try {
    for (const entry of pending) {
      const query = new URLSearchParams({
        filename: entry.file.name,
        file_last_modified_ms: String(entry.file.lastModified),
      });
      if (ui.growPhotoCaption.value.trim()) query.set("caption", ui.growPhotoCaption.value.trim());
      if (ui.growPhotoDate.value) query.set("taken_at", ui.growPhotoDate.value);
      try {
        const response = await fetch(`/api/grows/${editingGrow.tub_id}/photos?${query.toString()}`, {
          method: "POST",
          headers: { "Content-Type": entry.mediaType },
          body: entry.file,
        });
        const responseText = await response.text();
        let data = {};
        try {
          data = responseText ? JSON.parse(responseText) : {};
        } catch {
          data = {};
        }
        if (!response.ok) throw new Error(data.detail || responseText || `HTTP ${response.status}`);
        uploaded.push(data.photo);
        URL.revokeObjectURL(entry.previewUrl);
      } catch (error) {
        failed.push({ entry, error });
      }
    }
    queuedPhotos = failed.map((item) => item.entry);
    editingGrow.photos.unshift(...uploaded);
    editingGrow.photo_count += uploaded.length;
    renderGrowPhotos(editingGrow.photos);
    renderPhotoQueue();
    if (failed.length) {
      ui.growError.textContent = `${failed.length} photo${failed.length === 1 ? "" : "s"} failed and remain queued: ${failed[0].error.message}`;
      ui.growError.hidden = false;
    } else {
      ui.growPhotoCaption.value = "";
      ui.growPhotoDate.value = "";
    }
    if (uploaded.length) {
      showToast(`${uploaded.length} photo${uploaded.length === 1 ? "" : "s"} uploaded and tagged.`);
    }
  } finally {
    ui.growUploadPhoto.disabled = queuedPhotos.length === 0;
  }
}

async function deleteGrowPhoto(photoId) {
  if (!editingGrow) return;
  try {
    const response = await fetch(`/api/grows/${editingGrow.tub_id}/photos/${photoId}`, { method: "DELETE" });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    editingGrow.photos = editingGrow.photos.filter((photo) => photo.photo_id !== photoId);
    editingGrow.photo_count = Math.max(0, editingGrow.photo_count - 1);
    renderGrowPhotos(editingGrow.photos);
    showToast("Photo deleted.");
  } catch (error) {
    ui.growError.textContent = error.message || "Could not delete photo.";
    ui.growError.hidden = false;
  }
}

function renderBreakShakeDates() {
  ui.jarBsList.replaceChildren();
  if (editingBreakShakeDates.length === 0) {
    const empty = document.createElement("span");
    empty.className = "date-chip-empty";
    empty.textContent = "No break-and-shake dates recorded.";
    ui.jarBsList.append(empty);
    return;
  }
  editingBreakShakeDates.forEach((value) => {
    const chip = document.createElement("span");
    chip.className = "date-chip";
    const text = document.createElement("span");
    text.textContent = formatDate(value);
    const remove = document.createElement("button");
    remove.type = "button";
    remove.textContent = "×";
    remove.disabled = Boolean(editingJar?.locked);
    remove.addEventListener("click", () => {
      editingBreakShakeDates = editingBreakShakeDates.filter((item) => item !== value);
      renderBreakShakeDates();
    });
    chip.append(text, remove);
    ui.jarBsList.append(chip);
  });
}

function clearJarPhotoQueue() {
  queuedJarPhotos.forEach((entry) => URL.revokeObjectURL(entry.previewUrl));
  queuedJarPhotos = [];
  ui.jarPhotoFile.value = "";
  renderJarPhotoQueue();
}

function renderJarPhotoQueue() {
  ui.jarPhotoQueue.replaceChildren();
  ui.jarUploadPhoto.textContent = queuedJarPhotos.length
    ? `Upload ${queuedJarPhotos.length} photo${queuedJarPhotos.length === 1 ? "" : "s"}`
    : "Upload queued";
  ui.jarUploadPhoto.disabled = queuedJarPhotos.length === 0 || !editingJar || editingJar.locked;
  if (queuedJarPhotos.length === 0) {
    const empty = document.createElement("div");
    empty.className = "photo-queue-empty";
    empty.textContent = "No photos queued.";
    ui.jarPhotoQueue.append(empty);
    return;
  }
  queuedJarPhotos.forEach((entry) => {
    const card = document.createElement("div");
    card.className = "photo-queue-card";
    const image = document.createElement("img");
    image.src = entry.previewUrl;
    image.alt = "";
    const details = document.createElement("div");
    const name = document.createElement("strong");
    name.textContent = entry.file.name;
    const size = document.createElement("span");
    size.textContent = `${(entry.file.size / (1024 * 1024)).toFixed(1)} MB`;
    details.append(name, size);
    const remove = document.createElement("button");
    remove.type = "button";
    remove.textContent = "×";
    remove.addEventListener("click", () => {
      URL.revokeObjectURL(entry.previewUrl);
      queuedJarPhotos = queuedJarPhotos.filter((item) => item.id !== entry.id);
      renderJarPhotoQueue();
    });
    card.append(image, details, remove);
    ui.jarPhotoQueue.append(card);
  });
}

function queueSelectedJarPhotos() {
  const supported = new Set(["image/jpeg", "image/png", "image/webp"]);
  const typeByExtension = new Map([
    ["jpg", "image/jpeg"], ["jpeg", "image/jpeg"],
    ["png", "image/png"], ["webp", "image/webp"],
  ]);
  const existing = new Set(queuedJarPhotos.map((entry) => entry.id));
  const rejected = [];
  [...ui.jarPhotoFile.files].forEach((file) => {
    const id = `${file.name}:${file.size}:${file.lastModified}`;
    if (existing.has(id)) return;
    const extension = file.name.split(".").pop().toLowerCase();
    const normalized = { "image/jpg": "image/jpeg", "image/pjpeg": "image/jpeg" }[file.type] || file.type;
    const mediaType = supported.has(normalized) ? normalized : (typeByExtension.get(extension) || "");
    if (!supported.has(mediaType) || file.size === 0 || file.size > (10 * 1024 * 1024)) {
      rejected.push(file.name);
      return;
    }
    existing.add(id);
    queuedJarPhotos.push({ id, file, mediaType, previewUrl: URL.createObjectURL(file) });
  });
  ui.jarPhotoFile.value = "";
  renderJarPhotoQueue();
  if (rejected.length) {
    ui.jarError.textContent = `${rejected.join(", ")} could not be queued. Use JPEG, PNG, or WebP files up to 10 MB.`;
    ui.jarError.hidden = false;
  }
}

function renderJarPhotos(photos, { historical = false } = {}) {
  const gallery = historical ? null : ui.jarPhotoGallery;
  if (gallery) gallery.replaceChildren();
  if (!historical) ui.jarPhotoCount.textContent = `${photos.length} photo${photos.length === 1 ? "" : "s"}`;
  const fragment = document.createDocumentFragment();
  if (photos.length === 0) {
    const empty = document.createElement("div");
    empty.className = "photo-empty";
    empty.textContent = "No photos uploaded for this jar.";
    fragment.append(empty);
  } else {
    photos.forEach((photo) => {
      const card = document.createElement("article");
      card.className = "photo-card";
      const image = document.createElement("img");
      image.src = `/api/jar-photos/${photo.photo_id}`;
      image.alt = photo.caption || photo.original_name;
      image.loading = "lazy";
      const copy = document.createElement("div");
      copy.className = "photo-card-copy";
      const caption = document.createElement("strong");
      caption.textContent = photo.caption || photo.original_name;
      const captured = document.createElement("span");
      captured.textContent = `${formatCaptureTime(photo)} - ${captureSourceLabel(photo.capture_time_source)}`;
      copy.append(caption, captured);
      card.append(image, copy);
      if (!historical && !editingJar?.locked) {
        const remove = document.createElement("button");
        remove.className = "photo-delete";
        remove.type = "button";
        remove.textContent = "×";
        remove.addEventListener("click", () => deleteJarPhoto(photo.photo_id));
        card.append(remove);
      }
      fragment.append(card);
    });
  }
  if (gallery) gallery.append(fragment);
  return fragment;
}

function setJarEditorLocked(locked) {
  ui.jarLockedBanner.hidden = !locked;
  ui.jarForm.querySelectorAll(".dialog-body input, .dialog-body textarea, .dialog-body select, .dialog-body button").forEach((element) => {
    element.disabled = locked;
  });
  ui.jarSave.disabled = locked;
  ui.jarCountInput.disabled = locked || Boolean(editingJar);
  ui.jarLockToggle.hidden = !editingJar || editingJar.status === "active";
  ui.jarLockToggle.textContent = locked ? "Unlock historical record" : "Lock historical record";
  ui.jarSpawn.hidden = !editingJar || editingJar.status !== "active";
  renderJarPhotoQueue();
  renderBreakShakeDates();
  renderJarPhotos(editingJar?.photos || []);
}

function populateJarDialog(jar) {
  editingJar = jar;
  editingBreakShakeDates = [...(jar?.break_shake_dates || [])];
  ui.jarDialogTitle.textContent = jar?.name || "New jars";
  ui.jarDialogStatus.textContent = jar
    ? jar.status === "active"
      ? "Current jar"
      : jar.status === "spawned"
        ? `Spawned to ${jar.spawned_to_tub_name || "tub"}`
        : jar.status === "failed"
          ? "Failed jar · contaminated"
          : "Archived jar"
    : "Create a current jar";
  ui.jarName.value = jar?.name || "";
  ui.jarCulture.value = jar?.culture || "";
  ui.jarSpecies.value = jar?.species || "";
  ui.jarGrain.value = jar?.grain_type || "";
  ui.jarCountInput.value = 1;
  ui.jarCountInput.disabled = Boolean(jar);
  ui.jarQuantityLabel.querySelector("small").textContent = jar
    ? "Each database record represents one physical jar."
    : "Creates separate, individually editable jar records.";
  ui.jarDryGrain.value = jar?.dry_grain_grams_per_jar ?? "";
  ui.jarPcTime.value = jar?.pressure_cooker_minutes ?? "";
  ui.jarPcPsi.value = jar?.pressure_psi ?? "";
  ui.jarPcDate.value = jar?.pressure_cooked_on || "";
  ui.jarInocDate.value = jar?.inoculated_on || "";
  ui.jarPrepTek.value = jar?.prep_tek || "";
  ui.jarNotes.value = jar?.notes || "";
  ui.jarBsDate.value = "";
  ui.jarPhotoDate.value = "";
  ui.jarPhotoCaption.value = "";
  ui.jarError.hidden = true;
  ui.jarLockedTitle.textContent = jar?.status === "failed"
    ? "Failed jar record locked"
    : jar?.status === "archived"
      ? "Archived jar record locked"
      : "Spawn record locked";
  ui.jarLockedDetail.textContent = jar?.status === "failed"
    ? [`Contaminated ${formatDate(jar.contaminated_on)}`, jar.lifecycle_reason]
      .filter(Boolean).join(" · ")
    : jar?.status === "archived"
      ? [`Archived ${formatDate(jar.archived_utc?.slice(0, 10))}`, jar.lifecycle_reason]
        .filter(Boolean).join(" · ")
      : "This historical record is protected. Unlock it only to correct old data.";
  clearJarPhotoQueue();
  setJarEditorLocked(Boolean(jar?.locked));
  ui.jarCountInput.disabled = Boolean(jar);
  const current = jar?.status === "active";
  ui.jarArchive.hidden = !current;
  ui.jarContaminated.hidden = !current;
  ui.jarDelete.hidden = !jar;
}

async function openJar(jarId) {
  try {
    const response = await fetch(`/api/jars/${jarId}`, { cache: "no-store" });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    populateJarDialog(data.jar);
    ui.jarDialog.showModal();
  } catch (error) {
    showToast(error.message || "Could not open jar.");
  }
}

function openNewJar() {
  populateJarDialog(null);
  ui.jarDialog.showModal();
}

function todayForInput() {
  const now = new Date();
  const offset = now.getTimezoneOffset() * 60000;
  return new Date(now.getTime() - offset).toISOString().slice(0, 10);
}

function openLifecycleDialog(kind, contaminated) {
  const record = kind === "grow" ? editingGrow : editingJar;
  if (!record) return;
  lifecycleAction = {
    kind,
    contaminated,
    id: kind === "grow" ? record.tub_id : record.jar_id,
    name: record.name,
  };
  const noun = kind === "grow" ? "grow" : "jar";
  ui.lifecycleTitle.textContent = contaminated
    ? `Mark ${noun} contaminated`
    : `Archive ${noun}`;
  ui.lifecycleSubtitle.textContent = record.name;
  ui.lifecycleDateLabel.firstChild.textContent = contaminated
    ? "Contamination date"
    : kind === "grow" ? "Finished date" : "Archive date";
  ui.lifecycleDate.value = todayForInput();
  ui.lifecycleReason.value = "";
  ui.lifecycleFirstFlush.value = "no";
  ui.lifecycleFlushLabel.hidden = !(kind === "grow" && contaminated);
  ui.lifecycleExplanation.textContent = kind === "grow" && contaminated
    ? "A contaminated grow is filed under Failed grows if it did not produce a first flush. If it produced at least one flush, it is retained under Past grows."
    : contaminated
      ? "This removes the jar from Current jars and files it under Failed jars."
      : kind === "grow"
        ? "This releases its sensor node and moves the complete record to Past grows."
        : "This removes the jar from Current jars and retains it under Archived jars.";
  ui.lifecycleConfirm.textContent = contaminated ? "Mark contaminated" : "Archive";
  ui.lifecycleConfirm.className = `button ${contaminated ? "caution" : "primary"}`;
  ui.lifecycleError.hidden = true;
  ui.lifecycleDialog.showModal();
}

async function submitLifecycle(event) {
  event.preventDefault();
  if (!lifecycleAction) return;
  ui.lifecycleConfirm.disabled = true;
  ui.lifecycleError.hidden = true;
  try {
    const base = lifecycleAction.kind === "grow" ? "grows" : "jars";
    const response = await fetch(`/api/${base}/${lifecycleAction.id}/archive`, {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        contaminated: lifecycleAction.contaminated,
        first_flush_harvested: ui.lifecycleFirstFlush.value === "yes",
        occurred_on: ui.lifecycleDate.value,
        reason: ui.lifecycleReason.value.trim(),
      }),
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    const message = lifecycleAction.contaminated
      ? `${lifecycleAction.name} marked contaminated and archived.`
      : `${lifecycleAction.name} archived.`;
    ui.lifecycleDialog.close();
    if (lifecycleAction.kind === "grow") ui.growDialog.close();
    else ui.jarDialog.close();
    lifecycleAction = null;
    showToast(message);
    await Promise.all([
      refreshDashboard(), refreshGrows(), refreshJars(), refreshArchive(), loadTubOptions(),
    ]);
  } catch (error) {
    ui.lifecycleError.textContent = error.message || "Could not archive record.";
    ui.lifecycleError.hidden = false;
  } finally {
    ui.lifecycleConfirm.disabled = false;
  }
}

async function deleteLifecycleRecord(kind) {
  const record = kind === "grow" ? editingGrow : editingJar;
  if (!record) return;
  const noun = kind === "grow" ? "tub/grow" : "jar";
  if (!window.confirm(`Permanently delete ${record.name}? This deletes its notes and photos and cannot be undone.`)) return;
  try {
    const base = kind === "grow" ? "grows" : "jars";
    const id = kind === "grow" ? record.tub_id : record.jar_id;
    const response = await fetch(`/api/${base}/${id}`, { method: "DELETE" });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    if (kind === "grow") ui.growDialog.close();
    else ui.jarDialog.close();
    showToast(`${record.name} ${noun} permanently deleted.`);
    await Promise.all([
      refreshDashboard(), refreshGrows(), refreshJars(), refreshArchive(), loadTubOptions(),
    ]);
  } catch (error) {
    const target = kind === "grow" ? ui.growError : ui.jarError;
    target.textContent = error.message || "Could not delete record.";
    target.hidden = false;
  }
}

function addBreakShakeDate() {
  const value = ui.jarBsDate.value;
  if (!value || editingBreakShakeDates.includes(value)) return;
  editingBreakShakeDates.push(value);
  editingBreakShakeDates.sort();
  ui.jarBsDate.value = "";
  renderBreakShakeDates();
}

function nullableNumber(input) {
  return input.value === "" ? null : Number(input.value);
}

function jarPayload() {
  return {
    name: ui.jarName.value.trim(),
    grain_type: ui.jarGrain.value.trim(),
    prep_tek: ui.jarPrepTek.value.trim(),
    pressure_cooker_minutes: nullableNumber(ui.jarPcTime),
    pressure_psi: nullableNumber(ui.jarPcPsi),
    dry_grain_grams_per_jar: nullableNumber(ui.jarDryGrain),
    jar_count: 1,
    pressure_cooked_on: ui.jarPcDate.value || null,
    inoculated_on: ui.jarInocDate.value || null,
    culture: ui.jarCulture.value.trim(),
    species: ui.jarSpecies.value.trim(),
    break_shake_dates: editingBreakShakeDates,
    notes: ui.jarNotes.value.trim(),
  };
}

async function saveJar(event) {
  event.preventDefault();
  if (editingJar?.locked) return;
  ui.jarSave.disabled = true;
  ui.jarError.hidden = true;
  try {
    const quantity = editingJar ? 1 : Number(ui.jarCountInput.value);
    if (!editingJar && quantity > 1 && queuedJarPhotos.length) {
      throw new Error("Create the individual jars first, then add photos to the specific jar they show.");
    }
    const payload = jarPayload();
    if (!editingJar) payload.quantity = quantity;
    const response = await fetch(editingJar ? `/api/jars/${editingJar.jar_id}` : "/api/jars/bulk", {
      method: editingJar ? "PUT" : "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    const created = !editingJar;
    editingJar = editingJar ? data.jar : data.jars[0];
    if (queuedJarPhotos.length) {
      await uploadJarPhotos({ quiet: true });
      if (queuedJarPhotos.length) return;
    }
    ui.jarDialog.close();
    showToast(created
      ? `${quantity} individual jar${quantity === 1 ? "" : "s"} created.`
      : "Jar saved.");
    await Promise.all([refreshDashboard(), refreshGrows(), refreshJars(), refreshArchive()]);
  } catch (error) {
    ui.jarError.textContent = error.message || "Could not save jar.";
    ui.jarError.hidden = false;
  } finally {
    ui.jarSave.disabled = Boolean(editingJar?.locked);
  }
}

async function uploadJarPhotos({ quiet = false } = {}) {
  if (!editingJar || editingJar.locked || queuedJarPhotos.length === 0) return;
  ui.jarUploadPhoto.disabled = true;
  ui.jarError.hidden = true;
  const failed = [];
  const uploaded = [];
  for (const entry of [...queuedJarPhotos]) {
    const query = new URLSearchParams({
      filename: entry.file.name,
      file_last_modified_ms: String(entry.file.lastModified),
    });
    if (ui.jarPhotoCaption.value.trim()) query.set("caption", ui.jarPhotoCaption.value.trim());
    if (ui.jarPhotoDate.value) query.set("taken_at", ui.jarPhotoDate.value);
    try {
      const response = await fetch(`/api/jars/${editingJar.jar_id}/photos?${query.toString()}`, {
        method: "POST", headers: { "Content-Type": entry.mediaType }, body: entry.file,
      });
      const text = await response.text();
      let data = {};
      try { data = text ? JSON.parse(text) : {}; } catch { data = {}; }
      if (!response.ok) throw new Error(data.detail || text || `HTTP ${response.status}`);
      uploaded.push(data.photo);
      URL.revokeObjectURL(entry.previewUrl);
    } catch (error) {
      failed.push({ entry, error });
    }
  }
  queuedJarPhotos = failed.map((item) => item.entry);
  editingJar.photos.unshift(...uploaded);
  editingJar.photo_count += uploaded.length;
  renderJarPhotos(editingJar.photos);
  renderJarPhotoQueue();
  if (failed.length) {
    ui.jarError.textContent = `${failed.length} photo${failed.length === 1 ? "" : "s"} failed and remain queued: ${failed[0].error.message}`;
    ui.jarError.hidden = false;
  } else {
    ui.jarPhotoCaption.value = "";
    ui.jarPhotoDate.value = "";
  }
  if (uploaded.length && !quiet) showToast(`${uploaded.length} jar photo${uploaded.length === 1 ? "" : "s"} uploaded.`);
  return { uploaded: uploaded.length, failed: failed.length };
}

async function deleteJarPhoto(photoId) {
  if (!editingJar || editingJar.locked) return;
  try {
    const response = await fetch(`/api/jars/${editingJar.jar_id}/photos/${photoId}`, { method: "DELETE" });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    editingJar.photos = editingJar.photos.filter((photo) => photo.photo_id !== photoId);
    editingJar.photo_count = Math.max(0, editingJar.photo_count - 1);
    renderJarPhotos(editingJar.photos);
    showToast("Jar photo deleted.");
  } catch (error) {
    ui.jarError.textContent = error.message || "Could not delete photo.";
    ui.jarError.hidden = false;
  }
}

async function toggleJarLock() {
  if (!editingJar || editingJar.status === "active") return;
  const unlocking = editingJar.locked;
  if (unlocking && !window.confirm("Unlock this historical spawn record? Only do this to correct old data.")) return;
  try {
    const response = await fetch(`/api/jars/${editingJar.jar_id}/lock`, {
      method: "PUT", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ locked: !editingJar.locked }),
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    editingJar = data.jar;
    setJarEditorLocked(editingJar.locked);
    showToast(editingJar.locked ? "Spawn record locked." : "Spawn record unlocked for correction.");
  } catch (error) {
    ui.jarError.textContent = error.message || "Could not change record lock.";
    ui.jarError.hidden = false;
  }
}

function openSpawnDialog(jarIds = null) {
  const ids = jarIds || (editingJar?.status === "active" ? [editingJar.jar_id] : []);
  const jars = ids.map((id) => cachedJars.find((jar) => jar.jar_id === id)).filter(Boolean);
  if (jars.length === 0) return;
  const missingCulture = jars.filter((jar) => !jar.culture.trim());
  if (missingCulture.length) {
    showToast(`Set a culture on ${missingCulture.map((jar) => jar.name).join(", ")} before spawning.`);
    return;
  }
  spawningJarIds = jars.map((jar) => jar.jar_id);
  ui.spawnSourceName.textContent = jars.length === 1
    ? jars[0].name
    : `${jars.length} selected jars: ${jars.map((jar) => jar.name).join(", ")}`;
  ui.spawnError.hidden = true;
  const grouped = new Map();
  jars.forEach((jar) => {
    const key = jar.culture.trim().toLocaleLowerCase();
    if (!grouped.has(key)) grouped.set(key, []);
    grouped.get(key).push(jar);
  });
  ui.spawnGroups.replaceChildren();
  for (const groupJars of grouped.values()) {
    const speciesValues = [...new Set(
      groupJars.map((jar) => jar.species.trim()).filter(Boolean),
    )];
    const normalizedSpecies = new Set(speciesValues.map((value) => value.toLocaleLowerCase()));
    if (normalizedSpecies.size > 1) {
      showToast(`${groupJars[0].culture} jars have mismatched species. Correct the jar records first.`);
      return;
    }
    const culture = groupJars[0].culture.trim();
    const species = speciesValues[0] || "Not specified";
    const card = document.createElement("section");
    card.className = "spawn-group-card";
    card.dataset.jarIds = groupJars.map((jar) => jar.jar_id).join(",");
    const heading = document.createElement("h3");
    heading.textContent = culture;
    const subtitle = document.createElement("p");
    subtitle.className = "spawn-group-subtitle";
    subtitle.textContent = `${groupJars.length} jar${groupJars.length === 1 ? "" : "s"}: ${groupJars.map((jar) => jar.name).join(", ")}`;
    const inherited = document.createElement("div");
    inherited.className = "spawn-inherited";
    inherited.append(makeJarFact(culture, "Strain / culture"), makeJarFact(species, "Species"));
    const fields = document.createElement("div");
    fields.className = "dialog-grid";
    const tubLabel = document.createElement("label");
    tubLabel.textContent = "Tub name";
    const tubName = document.createElement("input");
    tubName.dataset.field = "tub-name";
    tubName.maxLength = 64;
    tubName.required = true;
    tubName.value = `${culture} tub`;
    tubLabel.append(tubName);
    const nodeLabel = document.createElement("label");
    nodeLabel.textContent = "Sensor node";
    const node = document.createElement("select");
    node.dataset.field = "node";
    const later = document.createElement("option");
    later.value = "";
    later.textContent = "Assign later";
    node.append(later);
    cachedNodes.filter((item) => !item.tub_name).forEach((item) => {
      const option = document.createElement("option");
      option.value = String(item.node_id);
      option.textContent = `Node ${item.node_id} - ${item.name}`;
      node.append(option);
    });
    nodeLabel.append(node);
    const ratioLabel = document.createElement("label");
    ratioLabel.textContent = "Spawn ratio";
    const ratio = document.createElement("input");
    ratio.dataset.field = "spawn-ratio";
    ratio.maxLength = 32;
    ratio.required = true;
    ratio.placeholder = "1:2";
    ratioLabel.append(ratio);
    const dateLabel = document.createElement("label");
    dateLabel.textContent = "Spawn to bulk date";
    const date = document.createElement("input");
    date.type = "date";
    date.dataset.field = "date";
    date.required = true;
    date.value = localIsoDate();
    dateLabel.append(date);
    const notesLabel = document.createElement("label");
    notesLabel.className = "wide";
    notesLabel.textContent = "Tub notes";
    const notes = document.createElement("textarea");
    notes.dataset.field = "notes";
    notes.maxLength = 10000;
    notes.rows = 3;
    notesLabel.append(notes);
    fields.append(tubLabel, nodeLabel, ratioLabel, dateLabel, notesLabel);
    card.append(heading, subtitle, inherited, fields);
    ui.spawnGroups.append(card);
  }
  ui.spawnDialog.showModal();
}

async function confirmSpawnToTub(event) {
  event.preventDefault();
  if (spawningJarIds.length === 0) return;
  ui.spawnConfirm.disabled = true;
  ui.spawnError.hidden = true;
  try {
    const groups = [...ui.spawnGroups.querySelectorAll(".spawn-group-card")].map((card) => ({
      jar_ids: card.dataset.jarIds.split(",").map(Number),
      tub_name: card.querySelector('[data-field="tub-name"]').value.trim(),
      node_id: card.querySelector('[data-field="node"]').value
        ? Number(card.querySelector('[data-field="node"]').value) : null,
      spawn_ratio: card.querySelector('[data-field="spawn-ratio"]').value.trim(),
      spawn_to_bulk_on: card.querySelector('[data-field="date"]').value,
      notes: card.querySelector('[data-field="notes"]').value.trim(),
    }));
    const response = await fetch("/api/jars/spawn", {
      method: "POST", headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ groups }),
    });
    const data = await response.json();
    if (!response.ok) throw new Error(data.detail || `HTTP ${response.status}`);
    ui.spawnDialog.close();
    if (ui.jarDialog.open) ui.jarDialog.close();
    selectedJarIds.clear();
    const tubNames = data.grows.map((grow) => grow.name).join(", ");
    showToast(`${spawningJarIds.length} jar${spawningJarIds.length === 1 ? "" : "s"} rolled into ${tubNames}.`);
    await Promise.all([refreshDashboard(), refreshGrows(), refreshJars(), loadTubOptions()]);
  } catch (error) {
    ui.spawnError.textContent = error.message || "Could not create tub.";
    ui.spawnError.hidden = false;
  } finally {
    ui.spawnConfirm.disabled = false;
  }
}

function renderNodeList(container, emptyState, nodes) {
  container.replaceChildren(...nodes.map(makeNodeRow));
  container.hidden = nodes.length === 0;
  emptyState.hidden = nodes.length !== 0;
}

function renderNodes() {
  const query = ui.nodeFilter.value.trim().toLowerCase();
  const filtered = cachedNodes.filter((node) =>
    `${node.name} ${node.node_id}`.toLowerCase().includes(query),
  );
  renderNodeList(ui.overviewNodeList, ui.overviewEmpty, cachedNodes);
  renderNodeList(ui.nodesNodeList, ui.nodesEmpty, filtered);
}

function setReceiverState(receiver) {
  const connected = receiver.connected && receiver.verified;
  const detected = receiver.connected && !receiver.verified;
  const state = connected ? "Connected" : detected ? "Detected" : "Waiting";
  const port = receiver.port || "No USB receiver";

  ui.sidebarStatusLight.className = `status-light ${connected ? "online" : ""}`;
  ui.sidebarStatus.textContent = connected ? "Receiver connected" : detected ? "Receiver detected" : "Server online";
  ui.sidebarDetail.textContent = receiver.message || (connected ? port : "Receiver not connected");
  ui.receiverAlert.classList.toggle("connected", connected);
  ui.receiverAlertTitle.textContent = connected ? "USB receiver connected" : detected ? "USB receiver detected" : "Waiting for USB receiver";
  ui.receiverAlertDetail.textContent = connected
    ? `Receiving packets on ${port}. ${receiver.packets_received.toLocaleString()} packets received this run.`
    : detected
      ? `Opened ${port}; waiting for the first valid receiver record.`
      : "The web server is running and scanning for the receiver.";
  ui.receiverState.textContent = state;
  ui.receiverPort.textContent = port;
  ui.receiverFormState.value = state;
  ui.receiverFormPort.value = receiver.port || "Automatic detection";
  ui.receiverFirmware.value = receiver.firmware_version || "Unknown";
  ui.receiverResetFlags.value = formatResetFlags(receiver.reset_flags);
  ui.receiverRecordCount.textContent = receiver.records_received.toLocaleString();
  ui.receiverPacketCount.textContent = receiver.packets_received.toLocaleString();
  ui.receiverStoredCount.textContent = receiver.measurements_stored.toLocaleString();
  ui.receiverDuplicateCount.textContent = receiver.duplicates_ignored.toLocaleString();
  ui.commandSentCount.textContent = receiver.commands_sent.toLocaleString();
  ui.commandAckCount.textContent = receiver.config_acks_received.toLocaleString();
  ui.receiverFrequency.value = receiver.frequency_hz
    ? `${(receiver.frequency_hz / 1000000).toLocaleString()} MHz`
    : "915 MHz";
  ui.receiverModel.value = receiver.radio_model ? receiver.radio_model.toUpperCase() : "SX1262";
  ui.receiverRadioState.value = receiver.radio_state || "Waiting for receiver";
}

function formatResetFlags(flags) {
  if (flags === null || flags === undefined) return "Waiting for status";
  return `0x${Number(flags).toString(16).toUpperCase().padStart(8, "0")}`;
}

function makeDiagnostic(component, title, detail) {
  const item = document.createElement("article");
  item.className = "diagnostic-item";
  const kind = document.createElement("span");
  kind.className = "diagnostic-kind";
  kind.textContent = component;
  const heading = document.createElement("strong");
  heading.textContent = title;
  const message = document.createElement("span");
  message.textContent = detail;
  item.append(kind, heading, message);
  return item;
}

function renderDiagnostics(receiver, nodes) {
  const issues = [];
  if (!receiver.connected || !receiver.verified) {
    issues.push(["Receiver", "USB receiver unavailable", receiver.message || "The server is waiting for the receiver."]);
  }
  if (receiver.last_error) issues.push(["Server", "Receiver-service error", receiver.last_error]);
  if (receiver.last_radio_error) issues.push(["Receiver", "Radio error", receiver.last_radio_error]);
  if (Number(receiver.storage_errors) > 0) issues.push(["Server", "Packet storage errors", `${receiver.storage_errors} packet${Number(receiver.storage_errors) === 1 ? "" : "s"} could not be written during this run.`]);
  if (Number(receiver.invalid_records) > 0) issues.push(["Receiver", "Invalid USB records", `${receiver.invalid_records} invalid record${Number(receiver.invalid_records) === 1 ? "" : "s"} received during this run.`]);
  if ((Number(receiver.reset_flags) & 0x20000000) !== 0) issues.push(["Receiver", "Watchdog reset detected", `Latest reset flags: ${formatResetFlags(receiver.reset_flags)}.`]);
  nodes.forEach((node) => {
    const label = `${node.name} (Node ${node.node_id})`;
    if (!node.sensor_valid || (node.sensor_error && node.sensor_error !== "none")) issues.push(["Node", `${label}: sensor fault`, node.sensor_error || "Latest sensor reading was invalid."]);
    if (Number(node.sensor_failure_count) > 0) issues.push(["Node", `${label}: sensor failures`, `${node.sensor_failure_count} recoverable sensor failure${Number(node.sensor_failure_count) === 1 ? "" : "s"} since boot.`]);
    if (Number(node.radio_failure_count) > 0) issues.push(["Node", `${label}: radio failures`, `${node.radio_failure_count} local radio operation failure${Number(node.radio_failure_count) === 1 ? "" : "s"} since boot.`]);
    if ((Number(node.last_reset_flags) & 0x20000000) !== 0) issues.push(["Node", `${label}: watchdog reset`, `Latest reset flags: ${formatResetFlags(node.last_reset_flags)}.`]);
    const battery = batteryState(node.battery_voltage_v);
    if (["low", "critical", "dead"].includes(battery)) {
      const action = battery === "dead" ? "replace or recharge immediately" : battery === "critical" ? "replace or recharge soon" : "plan a replacement or recharge";
      issues.push(["Battery", `${label}: battery ${battery}`, `${formatBattery(node.battery_voltage_v)} — ${action}.`]);
    }
  });
  ui.diagnosticsList.replaceChildren(...issues.map((issue) => makeDiagnostic(...issue)));
  ui.diagnosticsList.hidden = issues.length === 0;
  ui.diagnosticsEmpty.hidden = issues.length !== 0;
  ui.diagnosticsCount.textContent = issues.length === 0 ? "All clear" : `${issues.length} active issue${issues.length === 1 ? "" : "s"}`;
  ui.diagnosticsSummary.textContent = issues.length === 0 ? "No active server, receiver, or node conditions need attention." : "These conditions are derived from the latest live status and node reports.";
}

async function refreshDashboard() {
  ui.refreshButton.disabled = true;
  try {
    const response = await fetch("/api/dashboard", { cache: "no-store" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const data = await response.json();
    const updated = new Date(data.generated_utc);

    setReceiverState(data.receiver);
    cachedNodes = data.nodes;
    renderDiagnostics(data.receiver, cachedNodes);
    ui.nodeCount.textContent = data.counts.nodes.toLocaleString();
    ui.measurementCount.textContent = data.counts.measurements.toLocaleString();
    ui.pendingCommandCount.textContent = data.counts.pending_commands.toLocaleString();
    ui.updatedTime.textContent = updated.toLocaleTimeString([], { hour: "numeric", minute: "2-digit", second: "2-digit" });
    ui.lastRefresh.textContent = `Updated ${updated.toLocaleTimeString()}`;
    ui.apiStatus.textContent = "Online";
    renderNodes();
  } catch (error) {
    ui.sidebarStatusLight.className = "status-light error";
    ui.sidebarStatus.textContent = "Server offline";
    ui.sidebarDetail.textContent = "API unavailable";
    ui.receiverAlertTitle.textContent = "Dashboard API unavailable";
    ui.receiverAlertDetail.textContent = "The page could not reach the MycoLogger server.";
    ui.apiStatus.textContent = "Offline";
    console.error("Dashboard refresh failed", error);
  } finally {
    ui.refreshButton.disabled = false;
  }
}

ui.navItems.forEach((item) => item.addEventListener("click", () => showView(item.dataset.view)));
ui.archiveTabs.forEach((tab) => tab.addEventListener("click", () => {
  ui.archiveTabs.forEach((item) => item.classList.toggle("active", item === tab));
  ui.archivePanels.forEach((panel) => panel.classList.toggle(
    "active", panel.dataset.archivePanel === tab.dataset.archiveTab,
  ));
}));
ui.openViewButtons.forEach((button) => button.addEventListener("click", () => showView(button.dataset.openView)));
ui.menuButton.addEventListener("click", () => ui.sidebar.classList.toggle("open"));
ui.accountButton.addEventListener("click", () => {
  const open = ui.accountDropdown.hidden;
  ui.accountDropdown.hidden = !open;
  ui.accountButton.setAttribute("aria-expanded", String(open));
});
ui.shutdownServer.addEventListener("click", async () => {
  if (!window.confirm("Save completed changes and shut down the MycoLogger server?")) return;
  ui.shutdownServer.disabled = true;
  try {
    await fetch("/api/server/shutdown", { method: "POST" });
    document.body.innerHTML = "<main class='empty-state'><strong>Server shut down safely.</strong><span>Start it again from PowerShell when ready.</span></main>";
  } catch {
    showToast("Could not request server shutdown.");
    ui.shutdownServer.disabled = false;
  }
});
ui.loginButton.addEventListener("click", () => showToast("Account login will be enabled with the planned authentication update."));
ui.nodeFilter.addEventListener("input", renderNodes);
ui.refreshButton.addEventListener("click", refreshDashboard);
ui.diagnosticsRefresh.addEventListener("click", refreshDashboard);
ui.settingsForm.addEventListener("submit", saveNodeSettings);
document.querySelector("#settings-close").addEventListener("click", () => ui.settingsDialog.close());
document.querySelector("#settings-cancel").addEventListener("click", () => ui.settingsDialog.close());
ui.growForm.addEventListener("submit", saveGrow);
document.querySelector("#grow-close").addEventListener("click", () => ui.growDialog.close());
document.querySelector("#grow-cancel").addEventListener("click", () => ui.growDialog.close());
document.querySelector("#grow-add-pin").addEventListener("click", addPinDate);
ui.growUploadPhoto.addEventListener("click", uploadGrowPhoto);
ui.growPhotoFile.addEventListener("change", queueSelectedPhotos);
ui.growArchive.addEventListener("click", () => openLifecycleDialog("grow", false));
ui.growContaminated.addEventListener("click", () => openLifecycleDialog("grow", true));
ui.growDelete.addEventListener("click", () => deleteLifecycleRecord("grow"));
ui.addJar.addEventListener("click", openNewJar);
ui.jarForm.addEventListener("submit", saveJar);
document.querySelector("#jar-close").addEventListener("click", () => ui.jarDialog.close());
document.querySelector("#jar-cancel").addEventListener("click", () => ui.jarDialog.close());
document.querySelector("#jar-add-bs").addEventListener("click", addBreakShakeDate);
ui.jarPhotoFile.addEventListener("change", queueSelectedJarPhotos);
ui.jarUploadPhoto.addEventListener("click", () => uploadJarPhotos());
ui.jarLockToggle.addEventListener("click", toggleJarLock);
ui.jarSpawn.addEventListener("click", () => openSpawnDialog());
ui.jarArchive.addEventListener("click", () => openLifecycleDialog("jar", false));
ui.jarContaminated.addEventListener("click", () => openLifecycleDialog("jar", true));
ui.jarDelete.addEventListener("click", () => deleteLifecycleRecord("jar"));
ui.spawnSelectedJars.addEventListener("click", () => openSpawnDialog([...selectedJarIds]));
ui.spawnForm.addEventListener("submit", confirmSpawnToTub);
document.querySelector("#spawn-close").addEventListener("click", () => ui.spawnDialog.close());
document.querySelector("#spawn-cancel").addEventListener("click", () => ui.spawnDialog.close());
ui.lifecycleForm.addEventListener("submit", submitLifecycle);
document.querySelector("#lifecycle-close").addEventListener("click", () => ui.lifecycleDialog.close());
document.querySelector("#lifecycle-cancel").addEventListener("click", () => ui.lifecycleDialog.close());
ui.browserHost.textContent = window.location.host;

refreshDashboard();
refreshGrows();
refreshJars();
refreshArchive();
window.setInterval(() => {
  refreshDashboard();
  refreshGrows();
  refreshJars();
  refreshArchive();
}, 5000);
