// 📋 GOOGLE APPS SCRIPT CHO HỆ THỐNG ĐIỂM DANH RFID
// Sao chép code này vào Google Apps Script

// 🔧 CẤU HÌNH
const DATABASE_SHEET_NAME = "Database";
const ATTENDANCE_SHEET_NAME = "Attendance_Log";

function doPost(e) {
  console.log("Received POST request");
  console.log("Content type:", e.postData.type);
  console.log("Raw contents:", e.postData.contents);
  
  try {
    let data;
    
    // Handle different content types
    if (e.postData.type === "application/json") {
      // Parse JSON data từ ESP32
      data = JSON.parse(e.postData.contents);
      console.log("Parsed JSON data:", data);
    } else if (e.postData.type === "application/x-www-form-urlencoded") {
      // Parse form data
      console.log("Parsing form data...");
      const formData = e.postData.contents;
      data = {};
      
      // Parse form parameters manually
      const params = formData.split('&');
      for (let param of params) {
        const [key, value] = param.split('=');
        data[decodeURIComponent(key)] = decodeURIComponent(value || '');
      }
      console.log("Parsed form data:", data);
    } else {
      // Try to parse as JSON anyway
      data = JSON.parse(e.postData.contents);
      console.log("Force parsed as JSON:", data);
    }
    
    console.log("Final action:", data.action);
    
    switch(data.action) {
      case "addAttendance":
        return addAttendanceRecord(data);
      case "getDatabase":
        console.log("Processing getDatabase via POST");
        return getDatabaseRecords();
      default:
        return ContentService
          .createTextOutput(JSON.stringify({
            status: "error",
            message: "Invalid action: " + data.action,
            receivedData: data
          }))
          .setMimeType(ContentService.MimeType.JSON);
    }
  } catch (error) {
    console.error("Error processing POST request:", error);
    return ContentService
      .createTextOutput(JSON.stringify({
        status: "error",
        message: "POST parsing error: " + error.toString(),
        rawData: e.postData.contents,
        contentType: e.postData.type
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}

function doGet(e) {
  // 🔍 DEBUG: Log all parameters để kiểm tra
  console.log("doGet called with parameters:", e.parameter);
  console.log("Query string:", e.queryString);
  console.log("Parameter keys:", Object.keys(e.parameter || {}));
  console.log("Parameter values:", Object.values(e.parameter || {}));
  
  // Xử lý GET request cho database sync
  const action = e.parameter.action;
  console.log("Extracted action:", action);
  
  if (action === "getDatabase") {
    console.log("Processing getDatabase request via GET...");
    return getDatabaseRecords();
  }
  
  // Check for empty parameters
  if (!e.parameter || Object.keys(e.parameter).length === 0) {
    console.log("No parameters received in GET request");
  }
  
  // Default response với debug info
  console.log("No valid action found, returning default response");
  return ContentService
    .createTextOutput(JSON.stringify({
      status: "success",
      message: "RFID Attendance System is running!",
      timestamp: new Date().toISOString(),
      debug: {
        receivedAction: action,
        allParameters: e.parameter,
        queryString: e.queryString,
        parameterKeys: Object.keys(e.parameter || {}),
        hasParameters: !!(e.parameter && Object.keys(e.parameter).length > 0)
      }
    }))
    .setMimeType(ContentService.MimeType.JSON);
}

function addAttendanceRecord(data) {
  try {
    const spreadsheet = SpreadsheetApp.getActiveSpreadsheet();
    
    // Tạo hoặc lấy sheet Attendance_Log
    let attendanceSheet = spreadsheet.getSheetByName(ATTENDANCE_SHEET_NAME);
    if (!attendanceSheet) {
      attendanceSheet = spreadsheet.insertSheet(ATTENDANCE_SHEET_NAME);
      // Tạo header cho sheet mới
      attendanceSheet.getRange(1, 1, 1, 6).setValues([
        ["STT", "Mã thẻ", "Họ tên", "Ngày", "Giờ", "Trạng thái"]
      ]);
      attendanceSheet.getRange(1, 1, 1, 6).setFontWeight("bold");
      attendanceSheet.getRange(1, 1, 1, 6).setBackground("#E8F0FE");
    }
    
    // Lấy số dòng tiếp theo
    const lastRow = attendanceSheet.getLastRow();
    const nextRow = lastRow + 1;
    const stt = lastRow; // STT bắt đầu từ 1 (trừ header)
    
    // Thêm dữ liệu mới
    const newRowData = [
      stt,
      data.cardId,
      data.name,
      data.date,
      data.time,
      data.status
    ];
    
    attendanceSheet.getRange(nextRow, 1, 1, 6).setValues([newRowData]);
    
    // Format dữ liệu
    if (data.status === "IN") {
      attendanceSheet.getRange(nextRow, 6).setBackground("#D4F4DD"); // Xanh lá nhạt
    } else if (data.status === "OUT") {
      attendanceSheet.getRange(nextRow, 6).setBackground("#FDE7E9"); // Đỏ nhạt
    }
    
    // Auto-resize columns
    attendanceSheet.autoResizeColumns(1, 6);
    
    console.log(`Added attendance record: ${data.name} - ${data.status} at ${data.date} ${data.time}`);
    
    return ContentService
      .createTextOutput(JSON.stringify({
        status: "success",
        message: `Attendance recorded: ${data.name} - ${data.status}`,
        timestamp: new Date().toISOString()
      }))
      .setMimeType(ContentService.MimeType.JSON);
      
  } catch (error) {
    console.error("Error adding attendance record:", error);
    return ContentService
      .createTextOutput(JSON.stringify({
        status: "error",
        message: error.toString()
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}

function getDatabaseRecords() {
  try {
    const spreadsheet = SpreadsheetApp.getActiveSpreadsheet();
    let databaseSheet = spreadsheet.getSheetByName(DATABASE_SHEET_NAME);
    
    if (!databaseSheet) {
      return ContentService
        .createTextOutput(JSON.stringify({
          status: "error",
          message: "Database sheet not found"
        }))
        .setMimeType(ContentService.MimeType.JSON);
    }
    
    // Lấy dữ liệu từ sheet (bỏ qua header row)
    const lastRow = databaseSheet.getLastRow();
    if (lastRow <= 1) {
      return ContentService
        .createTextOutput(JSON.stringify({
          status: "success",
          users: [],
          count: 0,
          message: "Database is empty"
        }))
        .setMimeType(ContentService.MimeType.JSON);
    }
    
    const data = databaseSheet.getRange(2, 1, lastRow - 1, 4).getValues();
    const users = [];
    
    for (let i = 0; i < data.length; i++) {
      const row = data[i];
      if (row[1] && row[2]) { // Kiểm tra có mã thẻ và tên
        users.push({
          stt: row[0],
          cardId: row[1].toString().trim(),
          name: row[2].toString().trim(),
          role: row[3] ? row[3].toString().trim() : ""
        });
      }
    }
    
    console.log(`Retrieved ${users.length} users from database`);
    
    return ContentService
      .createTextOutput(JSON.stringify({
        status: "success",
        users: users,
        count: users.length,
        timestamp: new Date().toISOString()
      }))
      .setMimeType(ContentService.MimeType.JSON);
      
  } catch (error) {
    console.error("Error getting database records:", error);
    return ContentService
      .createTextOutput(JSON.stringify({
        status: "error",
        message: error.toString()
      }))
      .setMimeType(ContentService.MimeType.JSON);
  }
}

function setupDatabase() {
  // 📇 HÀM KHỞI TẠO DATABASE SHEET (chạy 1 lần đầu)
  const spreadsheet = SpreadsheetApp.getActiveSpreadsheet();
  
  // Tạo hoặc lấy sheet Database
  let databaseSheet = spreadsheet.getSheetByName(DATABASE_SHEET_NAME);
  if (!databaseSheet) {
    databaseSheet = spreadsheet.insertSheet(DATABASE_SHEET_NAME);
    
    // Tạo header
    databaseSheet.getRange(1, 1, 1, 4).setValues([
      ["STT", "Mã thẻ", "Họ tên", "Chức vụ"]
    ]);
    databaseSheet.getRange(1, 1, 1, 4).setFontWeight("bold");
    databaseSheet.getRange(1, 1, 1, 4).setBackground("#FCE8B2");
    
    // Thêm dữ liệu mẫu
    const sampleData = [
      [1, "D46C1200", "Nguyen Van A", "Sinh viên"],
      [2, "EBE90F05", "Tran Thi B", "Sinh viên"],
      [3, "12345678", "Le Van C", "Giảng viên"]
    ];
    
    databaseSheet.getRange(2, 1, sampleData.length, 4).setValues(sampleData);
    databaseSheet.autoResizeColumns(1, 4);
  }
  
  console.log("Database setup completed");
}

function testScript() {
  // 🧪 HÀM TEST SCRIPT
  const testData = {
    action: "addAttendance",
    cardId: "D46C1200", 
    name: "Test User",
    status: "IN",
    date: "25/12/2024",
    time: "09:30:00"
  };
  
  return addAttendanceRecord(testData);
}

function initializeSheets() {
  // 🚀 HÀM KHỞI TẠO TOÀN BỘ SYSTEM (chạy 1 lần đầu)
  setupDatabase();
  
  const spreadsheet = SpreadsheetApp.getActiveSpreadsheet();
  
  // Tạo sheet Attendance_Log nếu chưa có
  let attendanceSheet = spreadsheet.getSheetByName(ATTENDANCE_SHEET_NAME);
  if (!attendanceSheet) {
    attendanceSheet = spreadsheet.insertSheet(ATTENDANCE_SHEET_NAME);
    attendanceSheet.getRange(1, 1, 1, 6).setValues([
      ["STT", "Mã thẻ", "Họ tên", "Ngày", "Giờ", "Trạng thái"]
    ]);
    attendanceSheet.getRange(1, 1, 1, 6).setFontWeight("bold");
    attendanceSheet.getRange(1, 1, 1, 6).setBackground("#E8F0FE");
    attendanceSheet.autoResizeColumns(1, 6);
  }
  
  console.log("All sheets initialized successfully");
} 